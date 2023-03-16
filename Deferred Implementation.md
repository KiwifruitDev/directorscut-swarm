# Alien Swarm Deferred Implementation

## Directory Structure

### Standard Paths

```text
src/materialsystem/
src/game/client/deferred/
src/game/server/deferred/
src/game/shared/deferred/
```

### Additional Shader Paths

```text
src/dx9sdk/
src/devtools/
```

## Changes

A few changes in standard files have been made.

All of them have comments like this:

```c++
// @Deferred - Biohazard
```

### Affected Files

The following files have been modified:

```text
src/game/client/cdll_client_int.cpp
src/game/client/viewrender.h
src/game/client/viewrender.cpp (Not all changes have comments, using diff is recommended)
src/game/client/flashlighteffect.h
src/game/client/flashlighteffect.cpp
src/game/server/gameinterface.cpp
src/public/renderparm.h
```

Most core tweaks and changes can be found in the following headers:

```text
src/materialsystem/swarmshaders/deferred_global_common.h
src/game/client/deferred/deferred_client_common.h
src/game/shared/deferred/deferred_shared_common.h
src/game/server/deferred/deferred_server_common.h
```
