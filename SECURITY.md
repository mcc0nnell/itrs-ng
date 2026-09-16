# Security Policy

iTRS NG specifications and reference code may influence identity resolution, routing, trust, privacy, and accessibility for real-time communications. The repository is research/reference software and must not be treated as production emergency-call software.

Please avoid publishing exploitable security details in public issues before maintainers have had a reasonable opportunity to assess them. Reports should include the affected component or specification section, impact, reproduction details, and proposed mitigation when known.

Security design assumes hostile federation boundaries, stale or malicious resource records, downgrade attempts, replay, endpoint impersonation, privacy-sensitive capability metadata, and resource churn.

A security change must not weaken the core authority boundary: accessibility-resource selection may not rewrite the authoritative PSAP supplied by the NG911 side of the system.
