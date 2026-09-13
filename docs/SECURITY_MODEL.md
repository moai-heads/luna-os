# Unified identity, domain, and service model

## Design decision

Users, services, containers, and permissions should be different views of one first-class kernel concept: an **execution domain**.

Do not make them four unrelated subsystems. Every process runs inside a domain that carries:

- an identity and delegated capabilities;
- resource namespaces and visibility boundaries;
- resource limits and accounting;
- a lifecycle/unit identity; and
- the policy context used for permission checks and audit records.

The names remain useful because they describe different facets:

- a **user** is primarily an identity;
- a **service** is primarily a lifecycle and dependency unit;
- a **container** is primarily a namespace/isolation boundary; and
- a **permission** is a policy decision over a subject, object, and operation.

They are unified by sharing one domain/context model, not by pretending the facets are identical.

## Core objects

### Principal

Represents “who” is acting:

- numeric user/group identity;
- supplementary groups;
- capability set;
- security label or policy tags; and
- credential-transition history for auditing.

Human logins and service accounts use the same principal representation. A service does not need a special permission mechanism merely because it has no interactive login.

### Domain

Represents “where and with what limits” a process runs:

- process/PID namespace;
- mount and filesystem root namespace;
- IPC namespace;
- network namespace;
- hostname/time namespace where needed;
- memory, CPU, task, file-descriptor, and I/O limits;
- parent/child relationship; and
- policy/audit identity.

A normal boot initially has one domain. A container is a domain with selected namespaces and limits changed or isolated from its parent.

### Unit

Represents “how the work lives”:

- service/job name;
- desired and current state;
- dependencies and ordering;
- restart/backoff policy;
- start/stop credentials; and
- logs and exit status.

A unit may supervise one process or a process tree. The service manager should be user space where practical; the kernel only needs primitives for process groups, signals, resource limits, and notifications.

### Process and thread

A process has references to:

```text
principal -> credentials and capabilities
 domain   -> namespaces, limits, accounting, policy context
 unit     -> lifecycle, dependencies, restart/log metadata
```

Threads share the process address space and normally share these references. Explicit credential or namespace transitions create a new controlled context rather than silently mutating unrelated processes.

### Object

Files, directories, devices, sockets, shared-memory objects, and services are protected objects. Each object may have:

- owner principal and group;
- POSIX mode bits;
- ACL or capability requirements;
- namespace visibility;
- optional security label; and
- audit metadata.

## Permission evaluation

Every protected operation should pass through one policy path:

```text
caller process
    -> effective principal/domain
    -> namespace visibility check
    -> capability/policy check
    -> owner/group/mode/ACL check
    -> resource-limit check
    -> operation or an explicit errno
    -> audit record when configured
```

This prevents separate, subtly incompatible checks for “users”, “containers”, and “services”. POSIX mode bits remain the first compatibility layer; capabilities, namespaces, labels, and limits extend it without replacing the basic model.

## POSIX-first implementation

Do not implement the entire container/service system before a user process can run. Add the model in layers:

### Layer 1 — single-domain POSIX base

- one root domain;
- uid/gid, supplementary groups, umask;
- owner/group/mode checks for files and devices;
- process groups and sessions;
- `getuid`, `getgid`, `set*id` policy, `setpgid`, `setsid`, `kill`; and
- a consistent `EACCES`/`EPERM` distinction.

### Layer 2 — resource and lifecycle primitives

- per-domain task, memory, descriptor, and CPU accounting;
- process-group and child-exit notifications;
- limits exposed through a small process API;
- service supervision in user space; and
- audit events for identity changes, exec, signals, mounts, and denied access.

### Layer 3 — namespaces and containers

- PID namespace;
- mount namespace with `chroot`/`pivot_root`-like semantics;
- IPC namespace;
- network namespace after networking exists; and
- explicit create/join/leave operations guarded by capabilities.

A container is then created by a service manager or launcher by constructing a domain, assigning a principal, attaching namespaces, setting limits, and starting a unit inside it.

### Layer 4 — policy expansion

- delegated capabilities instead of broad root checks;
- ACLs and optional mandatory labels;
- service-specific device access;
- per-domain resource quotas;
- signed or measured service identities if required; and
- centralized audit and policy inspection.

## Suggested internal shape

This is an architectural shape, not yet a frozen ABI:

```text
luna_domain
  identity: luna_principal
  namespaces: pid, mount, ipc, net, uts, time
  limits: tasks, memory, cpu, fds, io
  unit: lifecycle/dependency metadata
  parent: optional parent domain
  policy: capabilities, labels, audit hooks
```

Processes, open files, mounts, sockets, devices, and services should hold references to the domain objects they belong to. Reference counting and explicit teardown are required before domains can be destroyed safely.

## Service manager boundary

A service manager should be a privileged user-space process that:

1. creates or joins a domain;
2. assigns the service principal and capabilities;
3. establishes mounts, devices, environment, and limits;
4. starts the service process tree;
5. observes exits and resource events; and
6. restarts or stops the unit according to policy.

This keeps policy and dependency logic out of the kernel while giving the kernel one coherent security context for enforcement.

## Security invariants

- A process cannot access an object it cannot see in its domain, even if a path string names it.
- Identity changes are explicit, capability-checked, and visible to audit.
- Joining a domain is not equivalent to becoming its owner.
- A service restart receives a fresh, declared context rather than stale inherited privilege.
- Resource exhaustion in one domain cannot starve the kernel or bypass another domain's limits.
- The initial root domain is privileged for bootstrap but is not a reason to special-case every permission check.

## Relation to the POSIX plan

The immediate implementation target is Layer 1: basic uid/gid/mode semantics, process groups, sessions, and a single root domain. Layers 2–4 follow after user mode, VFS, TTYs, signals, and resource accounting exist.

This makes the system useful as a small POSIX environment while leaving a clean path to containers and service isolation. BusyBox, if revisited later, should consume this model through ordinary syscalls and libc rather than receive a separate “container” or “service” port.
