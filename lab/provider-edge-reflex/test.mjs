import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { resolveEdge } from './edge.mjs';

const policy = JSON.parse(await readFile(new URL('./policy.json', import.meta.url)));

const directBinding = {
  identifier: '+12025550147',
  authority: { source: 'synthetic-number-registry' },
  endpoints: [
    { uri: 'sip:alice@direct.example', media: ['video'], language: 'ASL', state: 'available' },
  ],
};

const voiceBinding = {
  identifier: '+13015550100',
  authority: { source: 'synthetic-number-registry' },
  endpoints: [
    { uri: 'sip:+13015550100@voice.example', media: ['voice'], state: 'available' },
  ],
};

const providers = [
  {
    id: 'vrs-b', endpoint: 'sip:queue-b@vrs.example', language: 'ASL', media: ['video'],
    role: 'interpreter', state: 'available', scope: { jurisdiction: [] },
  },
  {
    id: 'vrs-a', endpoint: 'sip:queue-a@vrs.example', language: 'ASL', media: ['video'],
    role: 'interpreter', state: 'available', scope: { jurisdiction: [] },
  },
];

const directRequest = {
  target: directBinding.identifier,
  requested: { language: 'ASL', media: 'video' },
  emergency: false,
};

const direct = resolveEdge({ request: directRequest, binding: directBinding, resources: providers, policy });
assert.equal(direct.mode, 'direct');
assert.equal(direct.selected.endpoint, 'sip:alice@direct.example');
assert.equal(direct.legs.length, 1);
assert.equal(direct.candidates.some((c) => c.id === 'vrs-a'), false, 'provider must not enter direct path');

const interpretedRequest = {
  target: voiceBinding.identifier,
  requested: { language: 'ASL', media: 'video' },
  emergency: false,
};

const interpreted = resolveEdge({ request: interpretedRequest, binding: voiceBinding, resources: providers, policy });
assert.equal(interpreted.mode, 'interpreted');
assert.equal(interpreted.selected.id, 'vrs-a', 'stable id is deterministic tie-breaker');
assert.equal(interpreted.legs[1].endpoint, 'sip:+13015550100@voice.example');

const fallback = resolveEdge({
  request: interpretedRequest,
  binding: voiceBinding,
  resources: providers.map((p) => p.id === 'vrs-a' ? { ...p, state: 'unavailable' } : p),
  policy,
});
assert.equal(fallback.selected.id, 'vrs-b');
assert.notEqual(fallback.evidence.input_digest, interpreted.evidence.input_digest, 'resource-state change must change the input digest');
assert.notEqual(fallback.evidence.decision_digest, interpreted.evidence.decision_digest, 'resource-state change must change the decision digest');

const emergencyRequest = {
  target: voiceBinding.identifier,
  requested: { language: 'ASL', media: 'video' },
  emergency: true,
  authoritative_psap: 'sip:psap-md@example.invalid',
  policy_context: { jurisdiction: 'MD' },
};

const emergencyResources = [
  ...providers,
  {
    id: 'md-asl-tc', endpoint: 'sip:asl-tc@md.example', language: 'ASL', media: ['video'],
    role: 'telecommunicator', state: 'available', scope: { jurisdiction: ['MD'] },
  },
];
const emergency = resolveEdge({ request: emergencyRequest, binding: voiceBinding, resources: emergencyResources, policy });
assert.equal(emergency.mode, 'emergency-accessibility');
assert.equal(emergency.selected.id, 'md-asl-tc');
assert.equal(emergency.authority.psap, emergencyRequest.authoritative_psap, 'edge must preserve PSAP authority');
assert.equal(emergency.legs[0].endpoint, emergencyRequest.authoritative_psap);

const again = resolveEdge({ request: interpretedRequest, binding: voiceBinding, resources: providers, policy });
assert.equal(again.evidence.decision_digest, interpreted.evidence.decision_digest, 'same inputs must produce same decision digest');

console.log('provider-edge-reflex: 5/5 PASS');
console.log(`direct: ${directRequest.target} -> ${direct.selected.endpoint}`);
console.log(`interpreted: ${interpretedRequest.target} -> ${interpreted.selected.id} -> voice destination`);
console.log(`emergency: PSAP preserved -> ${emergency.selected.id} attached`);
console.log(`decision digest: ${interpreted.evidence.decision_digest}`);
