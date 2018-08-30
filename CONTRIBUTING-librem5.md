Contributing to the Librem5 version
===================================

The Librem5 version of wlroots carries additional patches, contained in various branches. This document explains the purpose of each branch and the preferred way to add changes to them.

Branches
--------

* *librem5*: Librem5 specific modifications. *master* and *f/<feature>* branches should be merged into *librem5*.
* *master*: tracks upstream's master. No branches get ever merged in here.
* *f/<feature>*: implements a certain feature.
* *debian/sid*: The branch used for Debian packaging. Only *librem5* merges in here. Never make any changes outside of *debian/* here.

Updating from upstream
----------------------

Pull desired commit from upstream into master, and then merge it into *librem5*. Test your changes, then push the *librem5* branch.

Adding a new feature
--------------------

Test your feature. Once happy with it, try to merge it upstream and follow *updating from upstream*. If for some reason the change isn't getting upstreamed, then create a new *f/<feature>* branch and make a merge request against *librem5*.

Updating a feature
------------------

(Basically subsystem trees in Linux). Merge *librem5* into your *f/* branch when needed. Update you feature, do another merge request against *librem5* when done.

Updating the Debian packaging
-----------------------------

Merge *librem5* into the *debian/sid* branch. Use the script called `debian/update-git-snapshot` (`git-buildpackage` required, available in Debian).

