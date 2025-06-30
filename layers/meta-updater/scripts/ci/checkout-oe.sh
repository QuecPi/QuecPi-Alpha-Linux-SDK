#!/usr/bin/env bash

set -euo pipefail

set -x

REMOTE_SOURCE=${REMOTE_SOURCE:-https://github.com/uptane}
MANIFEST=${MANIFEST:-master}
CURRENT_PROJECT=${CURRENT_PROJECT:-}

# list of projects to pin to one version in the format:
# "project:rev;project2:rev2..."
PIN_LIST=${PIN_LIST:-}

#CURRENT_REV=$(git rev-parse HEAD)
LOCAL_REPO=$PWD

mkdir -p updater-repo

cd updater-repo

# it seems like it's always required now, even if repo is only used to fetch
git config --global user.email "meta-updater-ci@example.org"
git config --global user.name "meta-updater-ci"

if [ -d .repo/manifests ]; then
    git -C .repo/manifests reset --hard
fi

repo init -m "${MANIFEST}.xml" -u "$REMOTE_SOURCE/updater-repo"

# patch manifest:
# - add a new "uptane" remote that points to "$REMOTE_SOURCE"
# - change projects that contain "uptane" to use the uptane remote
MANIFEST_FILE=".repo/manifests/${MANIFEST}.xml"
xmlstarlet ed --omit-decl -L \
    -s "/manifest" -t elem -n "remote" -v "" \
    -i "/manifest/remote[last()]" -t attr -n "name" -v "uptane" \
    -i "/manifest/remote[last()]" -t attr -n "fetch" -v "$REMOTE_SOURCE" \
    -d "/manifest/project[contains(@name, 'uptane')]/@remote" \
    -i "/manifest/project[contains(@name, 'uptane')]" -t attr -n "remote" -v "uptane" \
    "$MANIFEST_FILE"

# hack: sed on `uptane/` names, to remove this unwanted prefix
sed -i 's#name="uptane/#name="#g' "$MANIFEST_FILE"

# pin projects from the list
(
IFS=";"
for pin in $PIN_LIST; do
    IFS=":"
    read -r project rev <<< "$pin"
    xmlstarlet ed --omit-decl -L \
        -d "/manifest/project[@name=\"$project\"]/@revision" \
        -i "/manifest/project[@name=\"$project\"]/@revision" -t attr -n "revision" -v "$rev" \
        -i "/manifest/project[@name=\"$project\"]" -t attr -n "revision" -v "$rev" \
        "$MANIFEST_FILE"
    IFS=";"
done
)

# Remove the current project from the manifest if we have it checked out
if [ -n "$CURRENT_PROJECT" ]; then
    xmlstarlet ed --omit-decl -L \
        -d "/manifest/project[@name=\"$CURRENT_PROJECT\"]" \
        "$MANIFEST_FILE"
fi

repo manifest

# Try to clean up the repos in case they are messed up, but this returns an
# error code if the repos don't exist, which the subsequent repo sync will fix.
repo forall -c 'git reset --hard ; git clean -fdx' || true

repo sync -d --force-sync

if [ -n "$CURRENT_PROJECT" ]; then
    rm -f "$CURRENT_PROJECT"
    ln -s "$LOCAL_REPO" "$CURRENT_PROJECT"
fi

repo manifest -r
