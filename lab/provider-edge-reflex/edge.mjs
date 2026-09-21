import { createHash } from 'node:crypto';

const REQUIRED_MEDIA = new Set(['video', 'voice', 'rtt', 'text']);

function stable(value) {
  if (Array.isArray(value)) return value.map(stable);
  if (value && typeof value === 'object') {
    return Object.fromEntries(Object.keys(value).sort().map((k) => [k, stable(value[k])]));
  }
  return value;
}

export function canonicalJson(value) {
  return JSON.stringify(stable(value));
}

export function sha256(value) {
  return createHash('sha256').update(typeof value === 'string' ? value : canonicalJson(value)).digest('hex');
}

function assertRequest(request) {
  if (!request || typeof request !== 'object') throw new TypeError('request is required');
  if (typeof request.target !== 'string' || request.target.length === 0) throw new TypeError('target is required');
  if (!request.requested || !REQUIRED_MEDIA.has(request.requested.media)) throw new TypeError('requested.media is invalid');
  if (typeof request.requested.language !== 'string') throw new TypeError('requested.language is required');
  if (request.emergency && typeof request.authoritative_psap !== 'string') {
    throw new TypeError('emergency requests require authoritative_psap');
  }
}

function endpointSupports(endpoint, requested) {
  return endpoint.state !== 'unavailable'
    && endpoint.media?.includes(requested.media)
    && (!endpoint.language || endpoint.language === requested.language);
}

function directCandidate(binding, requested) {
  const endpoints = [...(binding.endpoints ?? [])]
    .filter((endpoint) => endpointSupports(endpoint, requested))
    .sort((a, b) => a.uri.localeCompare(b.uri));
  return endpoints[0] ?? null;
}

function scopeScore(resource, jurisdiction) {
  const scopes = resource.scope?.jurisdiction ?? [];
  if (!jurisdiction) return scopes.length === 0 ? 0 : 20;
  if (scopes.includes(jurisdiction)) return 0;
  if (scopes.length === 0) return 20;
  return 40;
}

function resourceRoleScore(resource, emergency) {
  if (emergency) {
    if (resource.role === 'telecommunicator') return 0;
    if (resource.role === 'interpreter') return 30;
    if (resource.role === 'bridge') return 40;
    return 90;
  }
  if (resource.role === 'interpreter') return 0;
  if (resource.role === 'bridge') return 10;
  return 80;
}

function evaluateResources(resources, request) {
  return resources.map((resource) => {
    const reasons = [];
    const media = resource.media?.includes('video');
    const language = resource.language === request.requested.language;
    const available = resource.state === 'available';
    const roleAllowed = request.emergency
      ? ['telecommunicator', 'interpreter', 'bridge'].includes(resource.role)
      : ['interpreter', 'bridge'].includes(resource.role);
    if (language) reasons.push('language-match');
    if (media) reasons.push('video-match');
    if (available) reasons.push('available');
    if (roleAllowed) reasons.push('role-allowed');
    if (request.policy_context?.jurisdiction && resource.scope?.jurisdiction?.includes(request.policy_context.jurisdiction)) {
      reasons.push('jurisdiction-match');
    }
    const eligible = Boolean(language && media && available && roleAllowed);
    const score = eligible
      ? resourceRoleScore(resource, request.emergency) + scopeScore(resource, request.policy_context?.jurisdiction)
      : 1000;
    return { resource, eligible, score, reasons };
  }).sort((a, b) => a.score - b.score || a.resource.id.localeCompare(b.resource.id));
}

function finalize(decision, request, policy) {
  const evidence = {
    resolver: 'itrs-edge-reflex',
    policy_version: policy.version,
    input_digest: sha256({ request, policy_version: policy.version }),
  };
  const withoutDigest = { ...decision, evidence };
  evidence.decision_digest = sha256(withoutDigest);
  return withoutDigest;
}

export function resolveEdge({ request, binding, resources = [], policy }) {
  assertRequest(request);
  if (!binding || binding.identifier !== request.target) throw new TypeError('binding does not match target');
  if (!policy?.version) throw new TypeError('policy.version is required');

  const direct = directCandidate(binding, request.requested);
  if (!request.emergency && direct) {
    return finalize({
      target: request.target,
      mode: 'direct',
      selected: { kind: 'endpoint', id: direct.uri, endpoint: direct.uri },
      legs: [
        { from: 'caller', to: 'destination', endpoint: direct.uri, media: request.requested.media },
      ],
      candidates: [{ id: direct.uri, kind: 'endpoint', eligible: true, reasons: ['capability-match', 'direct-preferred'] }],
      authority: { numbering: binding.authority ?? null },
    }, request, policy);
  }

  const evaluated = evaluateResources(resources, request);
  const selected = evaluated.find((candidate) => candidate.eligible);
  if (!selected) {
    return finalize({
      target: request.target,
      mode: 'unresolved',
      selected: null,
      legs: [],
      candidates: evaluated.map(({ resource, ...rest }) => ({ id: resource.id, role: resource.role, ...rest })),
      authority: request.emergency
        ? { psap: request.authoritative_psap, numbering: binding.authority ?? null }
        : { numbering: binding.authority ?? null },
    }, request, policy);
  }

  const resource = selected.resource;
  const destination = direct ?? [...(binding.endpoints ?? [])].sort((a, b) => a.uri.localeCompare(b.uri))[0] ?? null;
  const mode = request.emergency ? 'emergency-accessibility' : 'interpreted';
  const legs = request.emergency
    ? [
        { from: 'caller', to: 'authoritative-psap', endpoint: request.authoritative_psap, media: 'session' },
        { from: resource.role, to: 'session', endpoint: resource.endpoint, media: 'video' },
      ]
    : [
        { from: 'caller', to: resource.role, endpoint: resource.endpoint, media: 'video' },
        { from: resource.role, to: 'destination', endpoint: destination?.uri ?? request.target, media: destination?.media?.[0] ?? 'voice' },
      ];

  return finalize({
    target: request.target,
    mode,
    selected: { kind: 'resource', id: resource.id, role: resource.role, endpoint: resource.endpoint },
    legs,
    candidates: evaluated.map(({ resource, ...rest }) => ({ id: resource.id, role: resource.role, ...rest })),
    authority: request.emergency
      ? { psap: request.authoritative_psap, numbering: binding.authority ?? null }
      : { numbering: binding.authority ?? null },
  }, request, policy);
}
