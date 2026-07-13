# us-littlefs Microservice

An embedded Microservice that wraps the **littlefs** power-loss resilient
filesystem and exposes it as a plug & play, run-time isolated, access
controlled service for microcontrollers, backed by the eMR **NV Storage**
Universal Resource.

## Overview

This Microservice provides a small, self-contained filesystem service. It
receives filesystem requests over IPC, executes them against a `littlefs`
instance whose block device is backed by the eMR **NV Storage** resource, and
returns the results to the caller.

> ### Credits & License
>
> This Microservice embeds the open-source **littlefs** library.
>
> - **Project**: littlefs
> - **Owner / Account**: littlefs-project
> - **Repository**: https://github.com/littlefs-project/littlefs
> - **Pinned commit**: `6cb4e86540eca0d9ba62500a298385c9d863c8be`
> - **License**: BSD-3-Clause
>
> All credit for the filesystem implementation goes to the **littlefs-project**
> and its contributors. This Microservice is only an integration/wrapper layer.
> Please refer to the upstream repository for the full license text and
> copyright notices.

## Resource Requirement

> The **"NV Storage"** Universal Resource **MUST be assigned** to this
> Microservice. All block device operations (read/prog/erase) are performed
> through the eMR NV Storage APIs (`nvmem_getsize`, `nvmem_read`,
> `nvmem_write`, `nvmem_erase`, `nvmem_clear`). No direct hardware access is
> performed.

## Build and Run
 - Clone the open-source library and initialise the repository using `make init_repo`
    - It will clone the open-source library (see the version in the `Libs/libraries.txt`).
 -  Apply `Libs/0001-Microservice-Changes.patch`
    - Uses mock standard functions (us_memcpy, us_memset ..., see Source/us_std.c)
 - Copy the required `CPU Core SDK` from Microservice Store's Developer Dashboards to `Environment/CPU/`.
 - Generate a config file under `Configurations\`
 - Create a Microservice Store Package using `make package CONFIG=<CONFIG_NAME>`

## Running Test Simulator
You dont need and embedded target to run the tests. There is a simulator config where you can run the Microservice and a Test Application in the development environment (Linux).

Then run the simulator
 > make simulator CONFIG=sim

You should see the test results

```
$ make simulator CONFIG=sim

Running simulator (timeout 1s)...

========================================

    0 > littlefs Microservice. Version : 1.0
    1 > Container : Microservice Test User App
    1 > Test 1: format
    1   PASS (expected 0, actual 0)
    1 > Test 2: mount
    1   PASS (expected 0, actual 0)
    1 > Test 3: file_open (write/create/trunc)
    1   PASS (expected 0, actual 0)
    1 > Test 4: file_write
    1   PASS (expected 28, actual 28)
    1 > Test 5: file_close
    1   PASS (expected 0, actual 0)
    1 > Test 6: file_open (read)
    1   PASS (expected 0, actual 0)
    1 > Test 7: file_read and verify content
    1   PASS (read 28 bytes, content matches)
    1 > Test 8: file_close
    1   PASS (expected 0, actual 0)
    1 > Test 9: remove
    1   PASS (expected 0, actual 0)
    1 > Test 10: unmount
    1   PASS (expected 0, actual 0)
    1 > usTest : SUCCESS

========================================
Simulator run complete.

```
