#!/bin/bash
set -e

bash .utils/.check-workdir.sh

if [ "${1}" = "" ]; then
    shopt -s globstar
    subtrees=(**/.gitsubtree)
else
    subtrees=($*)
fi

for subtree in "${subtrees[@]}"; do
    if [[ "${subtree}" != */.gitsubtree ]]; then
        subtree="${subtree}/.gitsubtree"
    fi
    path="$(dirname "${subtree}")"
    echo -e "\n\nUpdating ${path}..."
    while read -u $remote repo branch subdir; do
        if [ "${repo:0:1}" = "#" ]; then
            continue
        fi
        if [ "${subdir}" = "/" ]; then
            exec {capture}>&1
            result="$(git subtree pull -P "${path}" "${repo}" "${branch}" -m "Merge ${path} from ${repo}" 2>&1 | tee /proc/self/fd/$capture)"
            bash .utils/.check-merge.sh "${path}" "${repo}" "${result}"
        else
            bash .utils/.subtree-subdir-helper.sh "${path}" "${repo}" "${branch}" "${subdir}" merge
        fi
    done {remote}< "${subtree}"
done

notify-send -t 0 -a Git -i git "Subtree update finished" "Double check merge commits" &> /dev/null | true
