===================
Unikraft Intrinsics
===================

The Unikraft build system relies on knowing the search paths or both the 
Unikraft core source code and any additional libraries.   By setting these 
directories, you can create a simple ``Makefile`` which acts as proxy into the
main build system.

To begin using Unikraft you'll need to have the following dependencies
installed: ::

  apt-get install -y --no-install-recommends build-essential libncurses-dev flex git wget bison unzip

Once these are installed, you can clone the Unikraft main repo: ::

  git clone http://github.com/unikraft/unikraft.git

If you'll be using Unikraft external libraries, this would be the time
to clone those too.  You can see a list of available libraries on `Github <https://github.com/unirkaft>`_
with the prefix ``lib-``.  You will need to clone each one separately.

We recommend the following directory structure for the Unikraft source code and
any additional libraries:  ::

  ├── unikraft/
  ├── apps/
  │  ├── app-1/
  │  ├── ...
  │  └── app-2/
  └── libs/
     ├── lib-1/
     ├── ...
     └── lib-N/

Once this is in place, you can create a ``Makefile`` inside your application
directory which uses these locations and uses the special Make target 
``$(MAKECMDGOALS)`` which returns the target used when calling ``make``: ::

  UK_ROOT ?= $(PWD)/../../unikraft
  UK_LIBS ?= $(PWD)/../../libs
  LIBS :=

  all:
      @$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)

  $(MAKECMDGOALS):
      @$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)

Now edit the Makefile in your application directory.  In particular, set the
``UK_ROOT`` and ``UK_LIBS`` variables to point to the directories where you
cloned the repos above.

Application structure
---------------------

To get quickly started, the easiest is to clone the hello world app (once again,
each Unikraft app has its own repo): ::

  git clone http://github.com/unikraft/app-helloworld.git

where your app is located at ``apps/helloworld``, you would set
those variables as follows: ::

  UK_ROOT ?= $(PWD)/../../unikraft
  UK_LIBS ?= $(PWD)/../../libs

Finally, if your app will be using external libraries, set the ``LIBS``
variable to reflect this. For instance : ::

  LIBS := $(UK_LIBS)/lib-1:$(UK_LIBS)/lib-2:$(UK_LIBS)/lib-N

.. note::
  
  The list of libraries must be colon-separated (``:``).

With all of this in place, we're now ready to start configuring the
application image via Unikraft's menu.  To access it, from within the
app's directory simply type ::

  make menuconfig

The menu system is fairly self-explanatory and will be familiar to
anyone who has configured a Linux kernel before. Select the options
you want, the libraries you'll like to include and don't forget to
select at least one platform (e.g., KVM, Xen or Linux user-space --
the latter is quite useful for quick testing and debugging).

Finally, quit the menu while saving the configuration changes you've
made and build your application by just typing ``make``. Unikraft will
then build each library in turn as well as the source files for your
application, producing one image in the ``./build`` directory for each
platform type you selected.

Running the image will depend on which platform you targeted. For
Linux user-space, for instance, the image is just a standard ELF, so
you can simply run it with ::

  ./build/helloworld_linuxu-x86_64

For more information regarding porting and developing apps (and
libraries) in Unikraft please read the developer's guide.
