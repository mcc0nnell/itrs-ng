# Celix ENUM provider plane

Add a ranked `org.itrsng.enum.provider` service contract, deterministic fixture provider, host-resolver NAPTR provider, and Number `resolveE164()` path with frozen observation provenance. Reject blocking ENUM resolution on the Celix event-loop thread and isolate DNS/provider locking from ASL-resource tracking.
