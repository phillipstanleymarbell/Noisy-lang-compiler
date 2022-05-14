# Welcome to the Noisy and Newton contributing guide

Thank you for taking the time to contribute to our project!

Please make sure to also read through the 
[Coding Conventions](https://github.com/phillipstanleymarbell/Noisy-lang-compiler/blob/master/README-CodingConventions.md).

This guide provides you with an overview of the development and contribution workflow for this project. The main topics it covers are processes
for opening an issue, creating a branch, creating a Pull Request, and getting it reviewed and merged.

## Getting started

Make sure you have the project set up correctly by following the guide in 
the [README.md](https://github.com/phillipstanleymarbell/Noisy-lang-compiler/blob/master/README.md).

## Issues and branches

This project follows a master and feature branches git workflow. The master branch contains the main version of the project. 
Feature branches branch off of the master branch, develop functionality, and are afterwards merged into the master branch.

**_Rule_**<br/>
Every feature branch must have a corresponding Issue that describes the changes the branch plans to make.

**_Branch name convention_**<br/>
Branch names must follow the syntax `issue-XXX` where `XXX` is the unique Issue number assigned (by GitHub) to the Issue describing the changes 
the branch plans to make.

The project installs a number of git-hooks that trigger for specific git operations. 
The [commitMessageHook.sh](https://github.com/phillipstanleymarbell/Noisy-lang-compiler/blob/master/hooks/commitMessageHook.sh) hook
automatically inserts a reference to the Issue that corresponds with the current git branch at the end of the commit message. This allows us to track
work towards a specific issue because GitHub will list these commits in the page of the Issue. This will only work correctly if branch names follow the 
project's branch name convention (see above) so please make sure that they do.

## Opening a Pull Request

When you are done making the changes that you planned to do, create a Pull Request against the master branch. A Pull Request means that the 
work is ready for review by a project maintainer; if it isn't but you're still seeking feedback/discussion on your work, consider 
opening a _Draft_ Pull Request.

- Don't forget to [link the PR the issue](https://docs.github.com/en/issues/tracking-your-work-with-issues/linking-a-pull-request-to-an-issue) that
corresponds to the branch.
- Enable the checkbox to [allow maintainer edits](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/allowing-changes-to-a-pull-request-branch-created-from-a-fork) 
so the branch can be updated for a merge.
- After you open the Pull Request, a project maintainer will review it. They may ask questions or request additional changes.
- Please make sure that your code follows the project's [Coding Conventions](https://github.com/phillipstanleymarbell/Noisy-lang-compiler/blob/master/README-CodingConventions.md).
- The maintainer will mark the Pull Request approved when it's ready.

## Your Pull Request is merged!

Congratulations! Thank you for your contributions!
You're now part of the Noisy and Newton languages community.

If you'd like to contribute more, you could take a look at the open Issues and pick something to help with.
If you have an idea that's not covered by an open Issue, you are very welcome to create a new Issue to present it and kick-off a discussion!

