=================
How to contribute
=================

Ignition Transport is an open source project based on the Apache License
Version 2.0, and is maintained by hardworking developers for everyone's benefit.
If you would like to contribute software patches, read on to find out how.

Development process
===================

We follow a development process designed to reduce errors, encourage
collaboration, and make high quality code. The process may seem rigid and
tedious, but every step is worth the effort (especially if you like
applications that work).

Steps to follow
---------------

1. Are you sure? Has your idea already been done, or maybe someone is already working on it?

  Check the `issue tracker <https://bitbucket.org/ignitionrobotics/ign-transport>`_.

2. `Fork Ignition Transport <https://bitbucket.org/ignitionrobotics/ign-transport/fork>`_. This will create your own personal copy of the project. All of your
development should take place in your fork.

3. Work out of a branch:

.. code-block:: bash

        hg branch my_new_branch_name

Always work out of a new branch, never off of `default`. This is a good habit to get in, and will make your life easier. If you're solving an issue, make the branch
name ``issue_`` followed by the issue number. E.g.: ``issue_23``.

4. Write your code.

  This is the fun part.

5. Write tests.

  A pull request will only be accepted if it has tests. See the `Test coverage` section below for more information.

6. Compiler warnings.

  Code must have zero compile warnings. This currently only applies to Linux.

7. Style.

  A tool is provided to check for correct style. Your code must have no errors
  after running the following command from the root of the source tree:

.. code-block:: bash

        sh tools/code_check.sh

The tool does not catch all style errors. See the `Style` section below for more
information.

8. Tests pass.

  There must be no failing tests. You can check by running ``make test`` in your
  build directory.

9. Documentation.

  Document all your code. Every class, function, member variable must have
  doxygen comments. All code in source files must have documentation that
  describes the functionality. This will help reviewers, and future developers.

10. Review your code.

  Before submitting your code through a pull request, take some time to review
  everything line-by-line. The review process will go much faster if you make
  sure everything is perfect before other people look at your code. There is a
  bit of the human-condition involved here. Folks are less likely to spend time
  reviewing your code if it's bad.

11. Small pull requests.

  A large pull request is hard to review, and will take a long time. It is worth
  your time to split a large pull request into multiple smaller pull requests.
  For reference, here are a few examples:

  * `Small, very nice <https://bitbucket.org/osrf/gazebo/pull-request/1732>`_
  * `Medium, still okay <https://bitbucket.org/osrf/gazebo/pull-request/1700>`_
  * `Too large <https://bitbucket.org/osrf/gazebo/pull-request/30>`_

12. `Pull request <https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/new>`_.

  Submit a pull request when you ready.

13. Review.

  At least two other people have to approve your pull request before it can be merged. Please be responsive to any questions and comments.

14. Done, phew.

  Once you have met all the requirements, you're code will be merged. Thanks for improving Ignition Transport!

Internal Developers
-------------------

This section is targeted mostly for people who have commit access to the main repositories.

In addition to the general development process, please follow these steps
before submitting a pull request. Each step is pass/fail, where the test or
check must pass before continuing to the next step.

1. Run the style checker on your personal computer.
2. Run all tests on your personal computer.
3. Run your branch through a jenkins `trusty build <http://build.osrfoundation.org/view/main/view/ignition/job/ignition_transport-ci-pr_any-trusty-amd64/>`_.
4. Run your branch through a jenkins `homebrew build <http://build.osrfoundation.org/view/main/view/ignition/job/ignition_transport-ci-pr_any-homebrew-amd64/>`_.
5. Run your branch through a jenkins `windows7 build <http://build.osrfoundation.org/view/main/view/ignition/job/ignition_transport-ci-pr_any-windows7-amd64/>`_.
6. Submit the pull request, and include the following:
  #. Link to a passing `trusty build <http://build.osrfoundation.org/view/main/view/ignition/job/ignition_transport-ci-pr_any-trusty-amd64/>`_.
  #. Link to a passing `homebrew build <http://build.osrfoundation.org/view/main/view/ignition/job/ignition_transport-ci-pr_any-homebrew-amd64/>`_.
  #. Link to a passing `windows7 build <http://build.osrfoundation.org/view/main/view/ignition/job/ignition_transport-ci-pr_any-windows7-amd64/>`_.
7. A set of jenkins jobs will run automatically once the pull request is created. Reviewers can reference these automatic jobs and the jenkins jobs listed in your pull request.

Style
-----

In general, we follow `Google's style guide <https://google-styleguide.googlecode.com/svn/trunk/cppguide.html>`_. However, we add in some extras.

**``this`` pointer**
   All class attributes and member functions must be accessed using the ``this->``  pointer. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/ gazebo/physics/Base.cc#cl-40>`_.

**Underscore function parameters**
   All function parameters must start with an underscore. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/Base.cc#cl-77>`_.

**Do not cuddle braces**
   All braces must be on their own line. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/Base.cc#cl-131>`_.

**Multi-line code blocks**
   If a block of code spans multiple lines and is part of a flow control statement, such as an ``if``, then it must be wrapped in braces. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/Base.cc#cl-249>`_

**++ operator**
   This occurs mostly in ``for`` loops. Prefix the ``++`` operator, which is `slightly more efficient than postfix in some cases <http://programmers.stackexchange.com/questions/59880/avoid-postfix-increment-operator>`_.

**PIMPL/Opaque pointer**
   If you are writing a new class, it must use a private data pointer. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/World.hh?at=default#cl-479>`_, and you can read more `here <https://en.wikipedia.org/wiki/Opaque_pointer>`_.

**const functions**
   Any class function that does not change a member variable should be marked as ``const``. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/Entity.cc?at=default#cl-175>`_.

**const parameters**
   All parameters that are not modified by a function should be marked as ``const``. This applies to parameters that are passed by reference, pointer, and value. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/Entity.cc?at=default#cl-217>`_.

**Pointer and reference variables**
   Place the ``*`` and ``&`` next to the variable name, not next to the type. For example: ``int &variable`` is good, but ``int& variable`` is not. Here is an `example <https://bitbucket.org/osrf/gazebo/src/default/gazebo/physics/Entity.cc?at=default#cl-217>`_.

**Camel case**
   In general, everything should use camel case. Exceptions include protobuf variable names.

**Class function names**
   Class functions must start with a capital letter, and capitalize every word.

   ``void MyFunction();`` : Good

   ``void myFunction();`` : Bad

   ``void my_function();`` : Bad

**Variable names**
   Variables must start with a lower case letter, and capitalize every word thereafter.

   ``int myVariable;`` : Good

   ``int myvariable;`` : Bad

   ``int my_variable;`` : Bad

Reduce Code Duplication
-----------------------

Check to make sure someone else is not currently working on the same
feature, before embarking on a project to add something to Ignition Transport.
Check the `issue tracker <https://bitbucket.org/ignitionrobotics/ign-transport/issues>`_ looking for issues with similar ideas.


Write Tests
-----------

All code should have a corresponding unit test. Ignition Transport uses `GTest <http://code.google.com/p/googletest>`_ for unit testing.

Test coverage
^^^^^^^^^^^^^

The goal is to achieve 100% line and branch coverage. However, this is not
always possible due to complexity issues, analysis tools misreporting
coverage, and time constraints. Try to write as complete of a test suite as
possible, and use the coverage analysis tools as guide. If you have trouble
writing a test please ask for help in your pull request.

Ignition Transport has a build target called ``make coverage`` that will produce a code coverage report. You'll need `lcov <http://ltp.sourceforge.net/coverage/lcov.php>`_  installed.

1. In your ``build`` folder, compile Ignition Transport with ``-DCMAKE_BUILD_TYPE=Coverage``:

.. code-block:: bash

        cmake -DCMAKE_BUILD_TYPE=Coverage ..\
        make

2. Run a single test, or all the tests:

.. code-block:: bash

        make test

3. Make the coverage report:

.. code-block:: bash

        make coverage

4. View the coverage report:

.. code-block:: bash

        firefox coverage/index.html

Debugging Ignition Transport
============================

Meaningful backtraces
---------------------

In order to provide meaningful backtraces when using a debugger, such as GDB, Ignition Transport should be compiled with debugging support enabled. When using the ubuntu packages, specially the ``-dbg`` package, this support is limited but could be enough in most of the situations. This are the three level of traces which can be obtained:

Maximum level of debugging support
   This only can be obtained compiling Ignition Transport from source and setting the ``CMAKE_BUILD_TYPE`` to ``DEBUG``. This will set up no optimizations and debugging symbols. It can be required by developers in situations specially difficult to reproduce.

Medium level of debugging support
   This can be obtained installing the ``libignition-transport1-dbg`` package or compiling Ignition Transport from source using the ``RELWITHDEBINFO`` ``CMAKE_BUILD_TYPE`` mode (which is the default if no mode is provided). This will set up ``-O2`` optimization level but provide debugging symbols. This should be the default when firing up gdb to explore errors and submit traces.

Minimum level of debugging support
   This one is present in package versions (no ``-dbg`` package present) or compiling Ignition Transport from source using the ``RELEASE`` ``CMAKE_BUILD_TYPE`` option. This will set up the maximum level of optimizations and does not provide any debugging symbol information. This traces are particularly difficult to follow.

Code Check
==========

Code pushed into the repository should pass a few simple tests. It is also helpful if patches submitted through bitbucket pass these tests. Passing these tests is defined as generating no error or warning messages for each of the following tests.


Static Code Check
-----------------

Static code checking analyzes your code for bugs, such as potential memory leaks, and style. The Ignition Transport static code checker uses cppcheck, and a modified cpplint. You'll need to install cppcheck on your system. Ubuntu users can install via:

.. code-block:: bash

        sudo apt-get install cppcheck

To check your code, run the following script from the root of the Ignition Transport sources:

.. code-block:: bash

        sh tools/code_check.sh

It takes a few minutes to run. Fix all errors and warnings until the output looks like:

.. code-block:: bash

        Total errors found: 0
