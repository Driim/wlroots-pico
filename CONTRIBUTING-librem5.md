Contributing to the Librem5 version
===================================

The Librem5 version of wlroots carries additional patches, contained in various branches. This document explains the purpose of each branch and the preferred way to add changes to them.

Branches
--------

* *master*: tracks upstream's master. No branches get ever merged in here.
* *librem5*: Librem5 specific modifications. *master* and *f/<feature>* branches should be merged into *librem5*.
* *f/<feature>*: implements a certain feature. Long lived.
* *debian/sid*: the branch used for Debian packaging. Only *librem5* merges in here. Never make any changes outside of *debian/* here.
* *freeform*: short-lived branches implementing fixes for any of the above, except *master*.

Updating from upstream
----------------------

Pull desired commit from upstream into master, and then merge it into *librem5*. Test your changes, then push the *librem5* branch.

Adding a new feature
--------------------

Test your feature. Once happy with it, try to merge it upstream and follow *updating from upstream*. If for some reason the change isn't getting upstreamed, then create a new *f/<feature>* branch and make a merge request against *librem5*.

Updating a feature
------------------

(Basically subsystem trees in Linux). Merge *master* into your *f/* branch when needed. Update your feature, do another merge request against *librem5* when done.

Fixing an issue
---------------

Create a new branch for your fix and then make a merge request against the branch which should receive it. Remove afterwards.

Updating the Debian packaging
-----------------------------

Merge *librem5* into the *debian/sid* branch. Use the script called `debian/update-git-snapshot` (`git-buildpackage` required, available in Debian).

Follow the [instructions on updating changelogs](https://www.debian.org/doc/debian-policy/ch-source.html#debian-changelog-debian-changelog). Use the newly created tag name as the *version*, and `experimental` as the *distribution*. Compose the changelog entry contents according to the [best practices](https://www.debian.org/doc/manuals/developers-reference/ch06.en.html#bpp-debian-changelog). Go to the root of the repository and use `dch edit` to do that.

