#!/bin/bash
set -e

bash .utils/.check-workdir.sh

if [ "$1" = "" ] || [ "$2" = "" ]; then
    echo "Usage 1: <path> <repo url> <branch> [subdir]"
    echo "Usage 2: <path> <repo url>/tree/<branch>[/subdir]"
    exit
fi
path="${1%/}"
repo="${2%/}"
if [ "$3" = "" ]; then
    read repo branch subdir <<< "$(sed -E "s|(https?://[^/]+)/([^/]+)/([^/]+)/(tree\|blob)/([^/]+)/?(.*)|\1/\2/\3 \5 \6|" <<< "${repo}")"
else
    branch="${3}"
    subdir="${4%/}"
fi
gitsubtree="${path}/.gitsubtree"

prevremotedir=""
if [ -e "${gitsubtree}" ]; then
    echo "Subtree already exists, adding new remote to it."
    prevremotedir="$(mktemp -d /tmp/gitsubtree-XXXXXXXX)"
    # To use 2 remotes for subtree we need to remove current one, add new one, then merge
    mv -T "${path}" "${prevremotedir}"
    git add "${path}"
    git commit -m "Add new remote for ${path}"
fi

if [ "${subdir}" = "" ]; then
    subdir="/"
    git subtree add -P "${path}" "${repo}" "${branch}" -m "Add ${path} from ${repo}"
else
    bash .utils/.subtree-subdir-helper.sh "${path}" "${repo}" "${branch}" "${subdir}" add
fi

if [ "${prevremotedir}" != "" ]; then
    if [ -e "${path}/.subtree-cache" ]; then
        # Backup subtree cache
        cp -rT "${path}/.subtree-cache" "${prevremotedir}/.subtree-cache"
    fi
    rm -r "${path}"
    mv -T "${prevremotedir}" "${path}"
fi

if [ -e "${gitsubtree}" ]; then
    # Add new remote at the top
    echo "${repo} ${branch} ${subdir}" | cat - "${gitsubtree}" > "${gitsubtree}.new"
    mv "${gitsubtree}.new" "${gitsubtree}"
else
    echo "${repo} ${branch} ${subdir}" > "${gitsubtree}"
fi
git add "${gitsubtree}"
git commit --amend --no-edit

if [ "${prevremotedir}" != "" ]; then
    prevremotedir=""
    echo "Added new remote for existing subtree, you must solve conflicts manually..."
fi
