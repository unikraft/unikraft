==============================
Getting started with ``kraft``
==============================

To begin using `Unikraft <https://unikraft.org>`_ you can use the command-line
utility ``kraft``  which is a companion tool used for defining, configuring,
building, and running Unikraft unikernel applications.  With ``kraft``, you can
create a build environment for your unikernel and manage dependencies for its
build.  Whilst ``kraft`` itself is an abstraction layer to the Unikraft build
system, it proves as a useful mechanic and starting ground for developing
unikernel applications.

Quick start
===========

``kraft`` can be installed by directly cloning its source from `GitHub <https://github.com/unikraft/kraft.git>`_: ::

  git clone https://github.com/unikraft/kraft.git
  cd kraft
  python3 setup.py install

.. note::
  Additional dependencies include `git`, `make`, ncurses, `flex`, `wget`,
  `unzip`, `tar`, `python3` (including  `setuptools`) and `gcc`.  Details on
  how to configure how ``kraft`` interacts with gcc and the Unikraft build
  system in addition on how to use ``kraft`` with Docker is covered in
  :ref:`advanced_usage`.

Once ``kraft`` it installed you can begin by initializing a new unikernel
repository using ``kraft init``.  As an example, you can build a Python 3
unikernel application by running the following: ::

  kraft list
  mkdir ~/my-first-unikernel && cd ~/my-first-unikernel
  kraft up -a helloworld -m x86_64 -p kvm

.. note::
  If this is the first time you are running ``kraft``, you will be prompted to 
  run an update which will download Unikraft core and additional library pool
  sources.  These sources are saved to directory set at the environmental
  variable ``UK_WORKDIR`` which defaults to ``~/.unikraft``.

With a newly initialized unikernel application, the ``./my-first-unikernel``
directory  will be populated with a ``deps.json`` file which contains references
to the the relevant library dependencies which are required to build a unikernel
with support for Python 3.  This file is used by ``kraft`` to configure and
build  against certain Unikraft library versions.  In addition to this file, a
new ``.config`` file will also be placed into the directory.  This file is used
by Unikraft's build system to switch on or off features depending on your
application's use case.

The unikernel application must now be configured against the Unikraft build
system so that you and it can resolve any additional requirements: ::

  kraft configure ./my-first-unikernel

.. note::
  This step can be made more interactive by launching into Unikraft's Kconfig
  configuration system.  Launch an ncurses window in your terminal with
  ``kraft configure --menuconfig``.

The configuration step used in ``kraft`` will perform necessary checks
pertaining to compatibility and availability of source code and will populate
your application directory with new files and folders, including:

  * ``kraft.yaml`` -- This file holds information about which version of the
    Unikraft core, additional libraries, which architectures and platforms to
    target and which network bridges and volumes to mount durirng runtime.
  * ``Makefile.uk`` -- A Kconfig target file you can use to create compile-time
    toggles for your application. 
  * ``build/`` -- All build artifacts are placed in this directory including 
    intermediate object files and unikernel images.
  * ``.config`` -- The selection of options for architecture, platform,
    libraries and your application (specified in ``Makefile.uk``) to use with
    Unikraft.

When your unikernel has been configured to your needs, you can build the
the unikernel to all relevant architectures and platforms using: ::

  kraft build ./my-first-unikernel

This step will begin the build process.  All artifacts created during this step
will be located under ``./my-first-unikernel/build``.

.. _kraft_cli:

Overview of commands
====================

::

  Usage: kraft [OPTIONS] COMMAND [ARGS]...

  Options:
    --version                       Show the version and exit.
    -C, --ignore-git-checkout-errors
                                    Ignore checkout errors.
    -X, --dont-checkout             Do not checkout repositories.
    -v, --verbose                   Enables verbose mode.
    -h, --help                      Show this message and exit.

  Commands:
    build      Build the application.
    clean      Clean the application.
    configure  Configure the application.
    init       Initialize a new unikraft application.
    list       List architectures, platforms, libraries or applications.
    run        Run the application.
    up         Configure, build and run an application.

  Influential Environmental Variables:
    UK_WORKDIR The working directory for all Unikraft
               source code [default: ~/.unikraft]
    UK_ROOT    The directory for Unikraft's core source
               code [default: $UK_WORKDIR/unikraft]
    UK_LIBS    The directory of all the external Unikraft
               libraries [default: $UK_WORKDIR/libs]
    UK_APPS    The directory of all the template applications
               [default: $UK_WORKDIR/apps]
    KRAFTCONF  The location of kraft's preferences file
               [default: ~/.kraftrc]

  Help:
    For help using this tool, please open an issue on Github:
    https://github.com/unikraft/kraft


.. _kraft_list:

Viewing Unikraft library pools
------------------------------

::

  Usage: kraft list [OPTIONS]

    Retrieves lists of available architectures, platforms, libraries and
    applications supported by unikraft.  Use this command if you wish to
    determine (and then later select) the possible targets for yourunikraft
    application.

    By default, this subcommand will list all possible targets.

  Options:
    -c, --core         Display information about Unikraft's core repository.
    -p, --plats        List supported platforms.
    -l, --libs         List supported libraries.
    -a, --apps         List supported application runtime execution
                       environments.
    -d, --show-local   Show local source path.
    -r, --show-origin  Show remote source location.
    -n, --paginate     Paginate output.
    -u, --update       Retrieves lists of available architectures, platforms
                       libraries and applications supported by Unikraft.
    -F, --flush        Cleans the cache and lists.
    -h, --help         Show this message and exit.


.. _kraft_up:

Quick Unikraft project creation
-------------------------------

::

  Usage: kraft up [OPTIONS] NAME

    Configures, builds and runs an application for a selected architecture and
    platform.

  Options:
    -p, --plat [linuxu|kvm|xen]    Target platform.
    -m, --arch [x86_64|arm|arm64]  Target architecture.
    -i, --initrd TEXT              Provide an init ramdisk.
    -X, --background               Run in background.
    -P, --paused                   Run the application in paused state.
    -g, --gdb INTEGER              Run a GDB server for the guest on specified
                                   port.
    -d, --dbg                      Use unstriped unikernel.
    -n, --virtio-nic TEXT          Attach a NAT-ed virtio-NIC to the guest.
    -b, --bridge TEXT              Attach a NAT-ed virtio-NIC an existing
                                   bridge.
    -V, --interface TEXT           Assign host device interface directly as
                                   virtio-NIC to the guest.
    -D, --dry-run                  Perform a dry run.
    -M, --memory INTEGER           Assign MB memory to the guest.
    -s, --cpu-sockets INTEGER      Number of guest CPU sockets.
    -c, --cpu-cores INTEGER        Number of guest cores per socket.
    -F, --force                    Overwrite any existing files in current
                                   working directory.
    -j, --fast                     Use all CPU cores to build the application.
    --with-dnsmasq                 Start a Dnsmasq server.
    --ip-range TEXT                Set the IP range for Dnsmasq.
    --ip-netmask TEXT              Set the netmask for Dnsmasq.
    --ip-lease-time TEXT           Set the IP lease time for Dnsmasq.
    -h, --help                     Show this message and exit.

.. _kraft_init:

Initializing a Unikraft project
-------------------------------

::

  Usage: kraft init [OPTIONS] NAME

    Initializes a new unikraft application.

    Start here if this is your first time using (uni)kraft.

  Options:
    -a, --app TEXT                 Use an existing application as a template.
    -p, --plat [linuxu|kvm|xen]    Target platform.
    -m, --arch [x86_64|arm|arm64]  Target architecture.
    -V, --version TEXT             Use specific Unikraft release version.
    -F, --force                    Overwrite any existing files.
    -h, --help                     Show this message and exit.


.. _kraft_configure:

Configuring a Unikraft application
----------------------------------

::

  Usage: kraft configure [OPTIONS]

  Options:
    -p, --plat [linuxu|kvm|xen]    Target platform.
    -m, --arch [x86_64|arm|arm64]  Target architecture.
    -F, --force                    Force writing new configuration.
    -k, --menuconfig               Use Unikraft's ncurses Kconfig editor.
    -h, --help                     Show this message and exit.


.. _kraft_build:

Building a Unikraft application
-------------------------------

::

  Usage: kraft build [OPTIONS]

    Builds the Unikraft application for the target architecture and platform.

  Options:
    -j, --fast  Use all CPU cores to build the application.
    -h, --help  Show this message and exit.

.. _kraft_run:

Running a Unikraft application
------------------------------

::

  Usage: kraft run [OPTIONS] [ARGS]...

  Options:
    -p, --plat [linuxu|kvm|xen]    Target platform.  [default: linuxu]
    -m, --arch [x86_64|arm|arm64]  Target architecture.  [default: (dynamic)]
    -i, --initrd TEXT              Provide an init ramdisk.
    -X, --background               Run in background.
    -P, --paused                   Run the application in paused state.
    -g, --gdb INTEGER              Run a GDB server for the guest at PORT.
    -d, --dbg                      Use unstriped unikernel.
    -n, --virtio-nic TEXT          Attach a NAT-ed virtio-NIC to the guest.
    -b, --bridge TEXT              Attach a NAT-ed virtio-NIC an existing
                                   bridge.
    -V, --interface TEXT           Assign host device interface directly as
                                   virtio-NIC to the guest.
    -D, --dry-run                  Perform a dry run.
    -M, --memory INTEGER           Assign MB memory to the guest.
    -s, --cpu-sockets INTEGER      Number of guest CPU sockets.
    -c, --cpu-cores INTEGER        Number of guest cores per socket.
    -h, --help                     Show this message and exit.

.. _advanced_usage:

Advanced Usage
==============

``kraft`` itself can be configured to meet the needs of your development
workflow.  If you are working directly the Unikraft source code or a library
then you can change ``kraft``'s behavior so that it recognizes changes which
you make.


.. _env_vars:

Influential environmental variables
-----------------------------------

``kraft`` uses environmental variables to determine the location of the Unikraft
core source code and all library pools.  This is set using the following:

+------------------------+--------------------------+-------------------------------------------+
| Environmental variable | Default value            | Usage                                     |
+========================+==========================+===========================================+
| ``UK_WORKDIR``         | ``~/.unikraft``          | The root directory for all sources.       |
+------------------------+--------------------------+-------------------------------------------+
| ``UK_ROOT``            | ``$UK_WORKDIR/unikraft`` | The Unikraft core source code.            |
+------------------------+--------------------------+-------------------------------------------+
| ``UK_LIBS``            | ``$UK_WORKDIR/libs``     | Library pool sources.                     |
+------------------------+--------------------------+-------------------------------------------+
| ``UK_APPS``            | ``$UK_WORKDIR/apps``     | Applications and templates.               |
+------------------------+--------------------------+-------------------------------------------+
| ``KRAFTCONF``          | ``~/.kraftrc``           | The location of kraft's preferences file. |
+------------------------+--------------------------+-------------------------------------------+

Workflow when working on Unikraft internals 
-------------------------------------------

During phases of development which require modifying the Unikraft core source
code or an auxiliary library for the target application, ``kraft``'s runtime
can be altered to facilitate varying developer requirements.

In the following example, both the Unikraft core source code and an additional
library, ``mylib``, have are utilized for an application.   However, their
source has been modified and point to external locations.  This is useful if you
are doing local development or wish to work with private repositories:

::

  specification: '0.4'

  unikraft: file:///home/developer/repos/unikraft/unikraft@3a8150d

  libraries:
    mylib:
      version: devel/new-feature
      source: git://git.example.com/lib-mylib


The ``kraft`` tool works with these remote and local Git repositories in order
to handle version control.  However, wen using the ``kraft`` tool itself in
during the ``configure`` and ``build`` steps, it is handy to stop it it from
automatically running ``git checkout`` on these repositories.  This is
particularly useful when the source tree of the Unikraft core or any other
library has a dirty working tree.

To ignore warnings and proceed with a command, use the global flag ``-C``:

::

  kraft -Cv configure

To prevent ``kraft`` from checking out repositories entirely, use the global
flag ``-X``:

::

  kraft -Xv build

Debugging Unikraft applications
-------------------------------

Running and debugging unikernels can be accomplished largely with the use of
`gdb <https://www.gnu.org/software/gdb/>`_.  Unikraft will build an un-stripped
binary with debugging features enabled.  This can be toggled with the
``-d|--dbg`` flag on ``kraft run``.  To start gdb itself, include the
``-g|--gdb PORT`` flag during the same run stage.  Additionally, it is often
useful to start the guest in a paused stage, accomplished with the
``-P|--paused`` flag:

::

  kraft run -p kvm --gdb 4123 --dbg
