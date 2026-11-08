import os
import git


def git_info_mm():
    submodule_list = []
    path = os.path.dirname(os.path.realpath(__file__))

    try:
        repo = git.Repo(path=path)
        repo_name = repo.remotes.origin.url.split('.git')[0].split('/')[-1]
        sha = repo.head.object.hexsha
    except git.exc.InvalidGitRepositoryError:
        repo_name = "No repository"
        sha = "N.A."

    return repo_name, sha, submodule_list


def git_info_mm_string():
    repo_name, sha, submodule_list = git_info_mm()
    string = "GIT tree:"
    string += "\n  >{}, SHA-1: {}".format(repo_name, sha)

    return string
