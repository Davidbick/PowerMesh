import os
import sys
import git

# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\MM_framework")

from MM_framework.mm_git_version import git_info_mm  # noqa: E402


def git_info_pf():
    submodule_list = []
    path = os.path.dirname(os.path.realpath(__file__))

    try:
        repo = git.Repo(path=path)
        repo_name = repo.remotes.origin.url.split('.git')[0].split('/')[-1]
        sha = repo.head.object.hexsha

        sub_repo_name, sub_sha, sub_submodule_list = git_info_mm()
        submodule_list.append([sub_repo_name, sub_sha, sub_submodule_list])
    except git.exc.InvalidGitRepositoryError:
        repo_name = "No repository"
        sha = "N.A."

    return repo_name, sha, submodule_list


def git_info_pf_string():
    repo_name, sha, submodule_list = git_info_pf()
    string = "GIT tree:"
    string += "\n  >{}, SHA-1: {}".format(repo_name, sha)
    string += submodule_string(submodule_list=submodule_list, base_tabs=1, level=1)

    return string


def submodule_string(submodule_list, base_tabs, level):
    string = ""
    for submodule in submodule_list:
        tabs = "  " * (base_tabs + level)
        string += "\n{}>{}, SHA-1: {}".format(tabs, submodule[0], submodule[1])
        if len(submodule[2]) > 0:
            string += submodule_string(submodule[2], base_tabs, level+1)

    return string
