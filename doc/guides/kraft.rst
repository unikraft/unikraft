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

``kraft`` can be installed by directly cloning its source from `GitHub <https://github.com/unikraft/tools.git>`_: ::

  git clone https://github.com/unikraft/tools.git
  cd tools && python setup.py install

.. note::
  Additional dependencies include `git`, `make`, ncurses, `flex`, `wget`,
  `unzip`, `tar`, `python3` and `gcc`.  Details on how to configure how
  ``kraft`` interacts with gcc and the Unikraft build system in addition on how
  to use ``kraft`` with Docker is covered in :ref:`advanced_usage`.

Once ``kraft`` it installed you can begin by initializing a new unikernel
repository using ``kraft init``.  As an example, you can build a Python 3
unikernel application by running the following: ::

  kraft init -a python3 ./my-first-unikernel

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

  * ``deps.json`` -- This file holds information about which version of the
    Unikraft core and additional libraries to use for the build.
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
    -v, --verbose  Enables verbose mode.
    -V, --version  Print the version and exit.
    --help         Show this message and exit.

  Commands:
    build      Build the unikraft appliance.
    configure  Sets the default configuration for an appliance.
    createfs   Generate a static filesystem for the unikraft appliance.
    init       Initialize a new unikraft project.
    list       List supported unikraft architectures, platforms, libraries or
               applications via remote repositories.
    run        Run the unikraft appliance.
    update     List supported unikraft architectures, platforms, libraries or
               applications via remote repositories.


.. _kraft_update:

Updating Unikraft library pools
-------------------------------

::

  Usage: kraft update [OPTIONS]

    This subcommand retrieves lists of available architectures, platforms,
    libraries and applications supported by unikraft.

  Options:
    -s, --staging  Use staging branch (here be dragons).
    --help         Show this message and exit.


.. _kraft_init:

Initializing a Unikraft project
-------------------------------

::

  Usage: kraft init [OPTIONS] [PATH] [NAME]

    This subcommand initializes a new unikraft application at a selected path.

    Start here if this is your first time using (uni)kraft.

  Options:
    -m, --arch TEXT  Target architecture  [default: (dynamic)]
    -p, --plat TEXT  Target platform  [default: linuxu]
    -l, --lib TEXT   Target platform
    -a, --app TEXT   Target application
    -F, --force      Overwrite any existing files.
    --help           Show this message and exit.


.. _kraft_configure:

Configuring a Unikraft application
----------------------------------

::

  Usage: kraft configure [OPTIONS] [PATH]

    This subcommand populates the local .config for the unikraft appliance
    with with the default values found for the target application.

  Options:
    -n, --menuconfig     Use Unikraft's ncurses Kconfig editor.
    -d, --dump-makefile  Write a Makefile compatible Unikraft's build system.
    -u, --dump-unikraft  Copy Unikraft and source libraries into the path.
    --help               Show this message and exit.



.. _kraft_build:

Building a Unikraft application
-------------------------------

::

  Usage: kraft build [OPTIONS] [PATH]

    This builds the unikraft appliance for the target architecture, platform
    and with all additional libraries and configurations.

  Options:
    -j, --fast  Use all CPU cores to build the application.
    --help      Show this message and exit.


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

+------------------------+--------------------------+------------------------------------+
| Environmental variable | Default value            | Usage                              |
+========================+==========================+====================================+
| ``UK_WORKDIR``         | ``~/.unikraft``          | The root directory for all sources |
+------------------------+--------------------------+------------------------------------+
| ``UK_ROOT``            | ``$UK_WORKDIR/unikraft`` | The Unikraft core source code      |
+------------------------+--------------------------+------------------------------------+
| ``UK_LIBS``            | ``$UK_WORKDIR/libs``     | Library pool sources               |
+------------------------+--------------------------+------------------------------------+
| ``UK_APPS``            | ``$UK_WORKDIR/apps``     | Applications and templates         |
+------------------------+--------------------------+------------------------------------+
