.. _rst_users_getting_started:

************************************
Getting Started
************************************

To begin using Unikraft you'll need a few packages. On Ubuntu/Debian
distributions run the command ::

  apt-get install bison flex build-essential python3 libncurses5-dev libncursesw5

Optionally, if you want to use the Python front-end to Unikraft's
menu, please also run ::

  apt-get install gtk+-2.0 gmodule-2.0 libglade-2.0

With this in place, we are ready to begin cloning Unikraft
repos. First, start by cloning the main Unikraft repo: ::

  git clone https://github.com/unikraft/unikraft.git

If you are thinking of using Unikraft external libraries, this would
be the time to clone those too. You can see a list of available
libraries at (any repo whose name has a ``lib-`` prefix): ::

  https://github.com/unikraft

As you can see, Each external library has its own separate repo, so
you'll need to clone each one separately. Likewise, if you are
planning on using any external platforms, please clone those too. You
can see a list of available external platforms at the same link as for
the libraries; the platform repos have a ``plat-`` prefix.

Finally, you'll need to create a Unikraft application. To quickly get
started, the easiest is to clone the hello world app (once again, each
Unikraft app has its own repo, this time with a ``app-`` prefix): ::

  git clone https://github.com/unikraft/app-helloworld.git

Now edit the Makefile in the app directory. In particular, set the
``UK_ROOT``, ``UK_LIBS``, and ``UK_PLATS`` variables to point to the
directories where you cloned the repos above. For instance, assuming
the following directory structure ::

  ├── unikraft
  ├── apps
  │  ├── helloworld
  │  ├── app1
  │  ├── app2
  │  ...
  │  ├── appN
  ├── libs
  │  ├── lib1
  │  ├── lib2
  │  ...
  │  └── libN
  └── plats
     ├── plat1
     ├── plat2
     ...
     └── platN

where your app is located at ``apps/helloworld``, you would set
the variables as follows: ::

  UK_ROOT  ?= $(PWD)/../../unikraft
  UK_LIBS  ?= $(PWD)/../../libs
  UK_PLATS ?= $(PWD)/../../plats

If your app uses external libraries, set the ``LIBS`` variable to
reflect this. For instance : ::

  LIBS := $(UK_LIBS)/lib1:$(UK_LIBS)/lib2:$(UK_LIBS)/libN

Note that the list has to be colon-separated.

Finally, if your app uses external platforms, set the ``PLATS``
variable: ::

  PLATS ?= $(UK_PLATS)/plat1:$(UK_PLATS)/plat2:$(UK_PLATS)/platN

Also make sure that you hand-over these platforms with the
``P=`` parameter to the sub make call in your main ``Makefile``: ::

  @make -C $(UK_ROOT) A=$(PWD) L=$(LIBS) P=$(PLATS) fetch
  @make -C $(UK_ROOT) A=$(PWD) L=$(LIBS) P=$(PLATS) prepare
  @make -C $(UK_ROOT) A=$(PWD) L=$(LIBS) P=$(PLATS)

With all of this in place, we're now ready to start configuring the
application image via Unikraft's menu. To access it, from within the
app's directory simply type ::

  make menuconfig

The menu system is fairly self-explanatory and will be familiar to
anyone who has configured a Linux kernel before. Select the options
you want, the libraries you'll like to include and don't forget to
select at least one platform (e.g., an external one, KVM, Xen, or
Linux user-space -- the latter is quite useful for quick testing and
debugging).

Finally, quit the menu while saving the configuration changes you've
made and build your application by just typing ``make``. Unikraft will
then build each library in turn as well as the source files for your
application, producing one image in the ``./build`` directory for each
platform type you selected.

Running the image will depend on which platform you targeted. For
Linux user-space, for instance, the image is just a standard ELF, so
you can simply run it with ::

  ./build/helloworld_linuxu-x86_64

For KVM, the following QEMU line should do the trick: ::

  qemu-system-x86_64 -cpu host -enable-kvm -m 128 -nodefaults -no-acpi -display none -serial stdio -device isa-debug-exit -kernel build/helloworld_kvm-x86_64

For Xen, create a ``helloworld.xen`` file as follows::

  kernel = "path_to_build_dir/build/helloworld_xen-x86_64"
  memory = "8"
  vcpus = "1"
  name = "helloworld"

  on_poweroff = "preserve"
  on_crash = "preserve"
  on_reboot = "preserve"

and instantiate the Xen virtual machine with: ::

  xl create -c helloworld.xen

That's it! For more information regarding porting and developing
apps (and libraries and platforms) in Unikraft please read the
developer's guide.
