# basetrade
[![Build Status](http://stratdev.qplum.co:8080/buildStatus/icon?job=qdata-testing)](http://stratdev.qplum.co:8080/job/qdata-testing/)


This package builds the trading strategy used for live trading and the
tools used for overnight processes related to live trading the next day.
We have not built it into a very usable package and neither higher
level classes and lower level classes are exposed.

# Table of contents
1. [Targets](#targets)
1. [How to run targets](#how-to-run-targets)
1. [Development Setup](#development-setup)
    * [Personal Github Setup](#personal-github-setup)
    * [Git setup](#git-setup)
    * [Moving from old git setup](#git-setup-when-you-have-previously-checked-out-cvquant)
    * [Compilation setup](#compilation-setup)
    * [Python Guidelines](#python-guidelines)
1. [Development workflow](#development-workflow)
    * [Raising pull requests](#raising-pull-request)
    * [Merging pull requests](#merging-pull-request)
    * [Overall PR workflow](#overall-pr-workflow)
1. [Test driven development](#continuous-integration)
1. [Future Work](#future-work)
1. [References](#references)

## Targets
The executables that are built are:
* [`tradeinit`](https://github.com/cvquant/basetrade/blob/devtrade/InitLogic/base_trade_init.cpp)
* [`sim_strategy`](https://github.com/cvquant/basetrade/blob/devmodel/InitLogic/sim_strategy.cpp)


## How-to-run-targets
* tradeinit: Running one-day's live trading on a given strategy file
Usage: ??
* `sim_strategy`: Running one days's backtest on a given strategy file
Usage: `~/basetrade_install/bin/sim_strategy SIM ~/basetrade/testbed/teststrategydescfile 1717 20101130`
* `datagen`:
* `timed_data_to_reg_data`:



## Development setup

### Personal github setup
1. `git config --list` to see your user fields.
1. `nano ~/.gitconfig` to put in the values you want to change

### Git setup
1. Fork the repository into your own user. For instance
[gauravchak/basetrade](https://github.com/gauravchak/basetrade) is forked from [cvquant/basetrade](https://github.com/cvquant/basetrade).
1. `git clone git@github.com/YOUR_USERNAME/basetrade.git basetrade`
After this if you do `cd basetrade` and run the command `git remote -v`, it should output:
      ```
      origin  git@github.com:YOUR_USERNAME/basetrade.git (fetch)
      origin  git@github.com:YOUR_USERNAME/basetrade.git (push)
      ```
1. `git remote add upstream git@github.com:cvquant/basetrade`
After this the following command `git remote -v` should produce:
      ```
      origin  git@github.com:YOUR_USERNAME/basetrade.git (fetch)
      origin  git@github.com:YOUR_USERNAME/basetrade.git (push)
      upstream  git@github.com:cvquant/basetrade.git (fetch)
      upstream  git@github.com:cvquant/basetrade.git (push)
      ```
1. `git submodule init`
1. `git submodule update`
1. `cd BASETRADE_HOME/dvctrade; git checkout master`, where if BASETRADE_HOME is the directory where you have checked out basetrade.
1. `cd BASETRADE_HOME/dvccode; git checkout master`
1. `cd BASETRADE_HOME; git checkout devmodel`

### Git setup when you have previously checked out cvquant
Most people have been working with cvquant/basetrade being checked out. To elaborate a `git remote -v` command prints:
```
origin  git@github.com:cvquant/basetrade.git (fetch)
origin  git@github.com:cvquant/basetrade.git (push)
```
	From this state, broadly you need to do the following steps.
1. Go to github.com/cvquant/basetrade and fork your repository.
1. `git remote rename origin upstream` changes cvquant to upstream
1. `git remote add origin got@github.com:YOUR_USERNAME/basetrade.git` adds your own fork as origin.
1. In case you are currently working in a branch CURRENT_BRANCH, `git branch --set-upstream-to origin/CURRENT_BRANCH CURRENT_BRANCH`

### Compilation setup
go to the main directory
`b2 release Initlogic//sim_strategy -j4` would build the target sim_strategy using a parallelization of 4.

### Python Guidelines

We follow `pep8` conventions with line-width 120 characters.
Overall focus is on the **simple code** with the main **goal of making
functions & classes easy to debug/maintain**.
Some guildelines that help achieve the above goal are:
* Small modular functions - Avoid writing very long functions.
  Long functions make it difficult to debug and test the logic.
* Break down long lines into multiple lines by assigning variables.
  Example:
  ```python
  # Difficult to read sentence
  df = pd.DataFrame(
      index=[chr(ord('A') + i) for i in range(1, 9, 2)],
      columns=[chr(ord('A') + i) for i in range(0, 8, 2)],
      data=np.random.randn(
          len([chr(ord('A') + i) for i in range(1, 9, 2)]), len([chr(ord('A') + i) for i in range(0, 8, 2)])))

  # It should be broken down into separate sentences.

  # Create the index list - 'B', 'D', 'F', 'H'
  index = [chr(ord('A') + i) for i in range(1, 9, 2)]
  # Create the columns list - 'A', 'C', 'E', 'G'
  columns = [chr(ord('A') + i) for i in range(0, 8, 2)]
  # Generate random data
  data = np.random.randn(len(index), len(columns))
  # Create the DataFrame
  df = pd.DataFrame(index=index, columns=columns, data=data)
  ```
* Avoid too many nested loops -
  instead carve them out into separate functions.
* After you finish writing the code, always ask yourself the question:
  **"Is this code easy enough for anyone in the team to understand and debug?"**
* Make sure to have no trailing spaces in your code.

## Development workflow
You should check out the code in your local computer and work on eclipse or PyCharm as the case might be.

### Raising pull-requests
1. In your fork create a new branch `git checkout -b NEW_BRANCH_NAME`
1. Commit your changes `git commit -a` < enter commit message >
1. Start a pull request from YOUR_USERNAME:new_branch_name to
`devmodel` or `devtrade` as the case may be.
1. After pull request the branch should be deleted.
1. To avoid people from using the same branch over and over again, do
   not merge a PR from the same branch name at least a month after the
   first merge, unless it is a permanent branch like `devmodel`.

### Merging pull-requests
1. You don't need to be an expert at reviewing PRs. If you are young,
   in the firm, the best way for you to get into the thick of things
   is to start merging PRs.
1. If you don't understand a PR becasue of lack of documentation,
   request changes, but do not merge.

### Overall PR workflow
1. We will not tag people in PRs unless two people are working together on a project
2. People should follow up when they see PR messages on #github
3. Please indicate when you have started a review
4. Approve and merge if it looks fine to you.

## Continuous Integration 
CI has been setup for basetrade repository
* Every pull request / branch will be built on a push. 
* Each repository would have a Jenkinsfile describing how the project needs to be built. 
* The entire process has been automated through Ansible, Terraform and Packer. 

Here are few snapshots:

1. In the comments section of the PR: 
![showing status of PR](docs/comments_section_PR_CI_basetrade.png?raw=true "showing status of PR") 

2. CI pipeline stage:
![showing status of CI](docs/CI_pipeine_page_CI_basetrade.png?raw=true "showing status of CI")

## Future-work
Chill out. Put your feet on the table and sip a glass of wine. That sounds cool right? Wonder if someone has a guitar!

## References
* [Git remotes](https://help.github.com/articles/configuring-a-remote-for-a-fork/#platform-linux)
