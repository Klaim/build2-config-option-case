# Case study (or issue repro.) for usage of configuration options in `build2`

This is project checks what is working or not with propagation of `config.somelib.someoption=value` package-specific options to their consumer projects.

### Context

```
$ b --version
build2 0.17.0-a.0.5745a2fe944d
libbutl 0.17.0-a.0.d249dff880ed
host x86_64-microsoft-win32-msvc14.3
Copyright (c) 2014-2023 the build2 authors.
This is free software released under the MIT license.


$ bdep --version
bdep 0.17.0-a.0.260c9251aa08
libbpkg 0.17.0-a.0.ca2934d78b84
libbutl 0.17.0-a.0.d249dff880ed
Copyright (c) 2014-2023 the build2 authors.
This is free software released under the MIT license.
```

## How to check:

```
git clone <this project's url>
cd myproject/
bdep init -C @myconfig cc
bdep test # FAILS by default
```
So far we just initialized the project in a  new configuration build. The project have 2 lbirarries, `libhello` which provide the hello-world implementation, and `hello` which uses `libhello`.

`hello` is setup to check at runtime if we have correctly set `libhello` in debug mode using the configuration option `config.libhello.debug=true`  (which is `false` by default).

After the above commands, the debug mode is not enabled, so the runtime tests (through `b test` or `bdep test`) will fail:
```
$ bdep test
mkdir ..\myproject-myconfig\libhello\fsdir{libhello\}
version libhello\libhello\in{version} -> ..\myproject-myconfig\libhello\libhello\hxx{version}
mkdir ..\myproject-myconfig\hello\fsdir{hello\}
mkdir ..\myproject-myconfig\libhello\tests\fsdir{basics\}
c++ hello\hello\cxx{hello} -> ..\myproject-myconfig\hello\hello\obje{hello}
c++ libhello\tests\basics\cxx{driver} -> ..\myproject-myconfig\libhello\tests\basics\obje{driver}
c++ libhello\libhello\cxx{hello} -> ..\myproject-myconfig\libhello\libhello\objs{hello}
ld ..\myproject-myconfig\libhello\libhello\libs{hello}
ld ..\myproject-myconfig\hello\hello\exe{hello}
ld ..\myproject-myconfig\libhello\tests\basics\exe{driver}
ln ..\myproject-myconfig\hello\hello\exe{hello} -> hello\hello\
ln ..\myproject-myconfig\libhello\tests\basics\exe{driver} -> libhello\tests\basics\
test ..\myproject-myconfig\hello\hello\exe{hello} + hello\hello\testscript{testscript}
test ..\myproject-myconfig\libhello\tests\basics\exe{driver}
hello\hello\testscript:3:1: error: process ..\myproject-myconfig\hello\hello\hello.exe terminated abnormally: aborted
  info: command line: E:\tools\build2\testing\config-value\myproject-myconfig\hello\hello\hello.exe World
  info: stderr: ..\myproject-myconfig\hello\hello\test-hello\basics\stderr
Assertion failed: hello::is_debug_mode() && "config.libhello.debug must be set to `true`", file E:\tools\build2\testing\config-value\myproject\hello\hello\hello.cxx, line 15
  info: test id: basics
  info: while testing ..\myproject-myconfig\hello\hello\exe{hello}
  info: while testing ..\myproject-myconfig\hello\dir{hello\}
  info: while testing ..\myproject-myconfig\dir{hello\}
ln ..\myproject-myconfig\hello\hello\test-hello -> hello\hello\
info: failed to test ..\myproject-myconfig\dir{hello\}
```

There are several ways to fix the situation and make the tests pass (depending on your preference and/or situation):
1. Set the configuration value in the very first `bdep init` command:
    ```
    bdep init -C @myconfig cc -- config.libhello.debug=true
    ```
    **In that specific command, beware of the `--` which indicates that the rest of the command is to be passed to the package manager (`bpkg`) to handle the configuration of packages correctly.
    Without it, the option will not be taken into account.**

2. Same as above, but you already have the configuration so you just want to empty it, then re-initialize the projects with the right configuration
    ```
    bdep deinit # This is optional, see below.
    bdep init config.libhello.debug=true
    ```
    **Note here that this is not the same init command than in 1) because `bdep` here already knows the configuration exists (it's stored in `.bdep/` so as long as you dont nuke it, the project knows about the configurations it have been initialized in in the past). This is why we dont need to re-create the configuration or to explicitly "add" to the configuration (`-A`)**

    Here `bdep deinit` just removes first the projects/packages from the build configuration, so it also removes any generated artefact, just to be sure. But in most cases it should work without this command.

3. Assuming you already have the projects initialized in the configuration: add the configuration change through the `bdep sync` command:
    ```
    bdep sync config.libhello.debug=true
    ```

Note that if you have multiple configuration (debug vs release etc. , multiple build toolchains, multiple target platforms, etc.) just add `--all` or `-a` to the above commands so that these commands applies on all the build configurations (you can list them using `bdep config list`)

4. Use a [Dependency Configuration](https://build2.org/release/0.15.0.xhtml#bpkg-dependency-configuration) for `hello` (see section below).

Once you have done one of the solutions above, `bdep test` (`-a`) should pass as expected, `b test` too.

NOTE: If you wanted to enable the option only to run the tests once, and not keep that option in following commands without explicitly re-running them, just pass the option to the test command:
```
bdep test config.libhello.debug=true   # PASS
bdep test                              # FAILS
```

## Gotchas about setting defines to propagate options:

### Read the recommandations in the manual

There is [a discussion specific to making libraries propagate their configuration options in the manual](https://build2.org/build2/doc/build2-build-system-manual.xhtml#proj-config-propag), it explains some of the issues you can encounter and how to avoid them.

### Config headers works best

As suggested in the above recommendations from the manual, I got less problems making this work correctly using the generation of a configuration header for `libhello`. So I recommend it too if you have more than one configuration option. I didnt keep that way of doing in here because I wanted the simplest case that doesnt work easilly, to demonstrate other issues.

### Not all export variable works as expected (or assumed)

I initially setup a define value for when the option is enabled, and exported it for the user-code to get the right version of the code. For exmaple, in `libhello/hello/buildfile`
```
...

if $config.libhello.debug
{
  ...
  cxx.poptions += -DLIBHELLO_DEBUG_MODE

  lib{hello}: cxx.export.poptions += -DLIBHELLO_DEBUG_MODE
}
```

However, this does not propagate correctly: `hello` builds command line does not contain `-DLIBHELLO_DEBUG_MODE` at all.
This is supposed to work as showed in the example from the manual which states:
> We can even use the same approach to export certain configuration information to our library's users (see Library Exportation and Versioning for details):
> ```
>     # libhello/buildfile
>
>     # Export options.
>     #
>     if $config.libhello.fancy
>       lib{hello}: cxx.export.poptions += -DLIBHELLO_FANCY
> ```
> This mechanism is simple and works well across compilers so there is no reason not to use it when the number of configuration values passed and their size are small.
>

As this approach does not work currently with my `build2` version (`build2 0.17.0-a.0.5745a2fe944d`), I suspect this is a bug.

However, explicitly adding to the export variables of each variation of the library type (instead of the group of types) does work:
```
if $config.libhello.debug
{
  ...
  cxx.poptions += -DLIBHELLO_DEBUG_MODE

  libs{hello}: cxx.export.poptions += -DLIBHELLO_DEBUG_MODE
  liba{hello}: cxx.export.poptions += -DLIBHELLO_DEBUG_MODE

}
```

Then once the option is set, it will be correctly propagated through the define when `hello` builds, and the runtime test will pass.

### Configuration loading

I failed to make configuration loading `config.config.load=with-libhello-debug.build` work (no option set anywhere), and I'm not sure why it does nothing. Maybe it works iwth `--option-file` but I didnt try yet.

## Clarification: Test sequences

```
git clone <this repo>
cd myproject/
bdep init -C @myconfig cc
bdep test                               # FAILS: config.libhello.debug == false
bdep sync config.libhello.debug=true
bdep test                               # PASS: config.libhello.debug == true
bdep test                               # PASS: config.libhello.debug's value is persisted for all further operations

bdep deinit                             # Removing all the projects from configurations will obviously also remove their configuration options
bdep init
bdep test                               # FAILS: config.libhello.debug == false
bdep init config.libhello.debug=true
bdep test                               # PASS: config.libhello.debug == true
bdep test                               # PASS: config.libhello.debug's value is persisted for all further operations

bdep deinit
bdep init
bdep test                               # FAILS: config.libhello.debug == false
bdep test config.libhello.debug=true    # PASS: config.libhello.debug == true , but only for the time of this command.
bdep test                               # FAILS: config.libhello.debug == false

rm -rf .bdep ../myproject-myconfig/     # Nuke everything, start from scratch, so that `bdep` does not rely on it's memory of configuration.
bdep init -C @myconfig cc -- config.libhello.debug=true  # Set the right config from the beginning (notice the `--`)
bdep test                               # PASS: config.libhello.debug == true

```


## Dependency Configuration

Since `build2 v0.15.0` we can [make a package specify a required configuration of their dependencies](https://build2.org/release/0.15.0.xhtml#bpkg-dependency-configuration).
This works as expected: when I set in `hello/manifest` (instead of `depends: libhello`):
```
depends:
\
libhello
{
  require
  {
    config.libhello.debug = true
  }
}
\
```

This will automatically set `config.libhello.debug=true` if you dont specify it yourself:
```
$ bdep init -C @myconfig cc
initializing in project E:\tools\build2\testing\config-value\myproject\
created configuration @myconfig E:\tools\build2\testing\config-value\myproject-myconfig\ 1 target default,forwarded,auto-synchronized
initializing package libhello
initializing package hello
synchronizing:
  new libhello/0.1.0-a.0.20231221221216
    config.libhello.debug=true (set by hello)
  new hello/0.1.0-a.0.20231221221216
libhello\libhello\buildfile:31:3: info: LIBHELLO DEBUG ENABLED

$ bdep test
libhello\libhello\buildfile:31:3: info: LIBHELLO DEBUG ENABLED
  hello\hello\buildfile:2:16: info: while loading export stub for lib{hello}
  info: while applying rule test to update (for test) ..\myproject-myconfig\dir{hello\}
mkdir ..\myproject-myconfig\libhello\fsdir{libhello\}
version libhello\libhello\in{version} -> ..\myproject-myconfig\libhello\libhello\hxx{version}
mkdir ..\myproject-myconfig\hello\fsdir{hello\}
mkdir ..\myproject-myconfig\libhello\tests\fsdir{basics\}
c++ libhello\libhello\cxx{hello} -> ..\myproject-myconfig\libhello\libhello\objs{hello}
c++ hello\hello\cxx{hello} -> ..\myproject-myconfig\hello\hello\obje{hello}
c++ libhello\tests\basics\cxx{driver} -> ..\myproject-myconfig\libhello\tests\basics\obje{driver}
ld ..\myproject-myconfig\libhello\libhello\libs{hello}
ld ..\myproject-myconfig\libhello\tests\basics\exe{driver}
ld ..\myproject-myconfig\hello\hello\exe{hello}
ln ..\myproject-myconfig\hello\hello\exe{hello} -> hello\hello\
ln ..\myproject-myconfig\libhello\tests\basics\exe{driver} -> libhello\tests\basics\
test ..\myproject-myconfig\hello\hello\exe{hello} + hello\hello\testscript{testscript}
test ..\myproject-myconfig\libhello\tests\basics\exe{driver}

```

As a demo, this is set in the `debug_by_default` branch.


