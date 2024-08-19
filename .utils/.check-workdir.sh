#!/bin/bash
set -e

if [ "$(git rev-parse --show-prefix)" != "" ]; then
    echo "Must be in root of git repo!"
    exit 1
fi
if [ "$(git branch --show-current)" = "" ]; then
    echo "Must be on a branch!"
    exit 1
fi
if ! git diff --quiet || ! git diff --cached --quiet || ! git merge HEAD &> /dev/null; then
    echo "Workdir must be clean!"
    exit 1
fi
