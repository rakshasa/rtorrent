#!/bin/bash
set -xe

# Clone rtorrent-docker and copy into test/docker/, then apply monorepo overrides.
# Usage: scripts/ci/setup-docker-tests.sh

RTORRENT_DOCKER_REPO="https://github.com/rakshasa/rtorrent-docker"
DOCKER_DIR="test/docker"
OVERRIDES_DIR="test/docker-overrides"

if [ ! -d "$DOCKER_DIR" ]; then
  git clone --depth 1 "$RTORRENT_DOCKER_REPO" /tmp/rtorrent-docker

  mkdir -p "$DOCKER_DIR"
  cp -a /tmp/rtorrent-docker/* "$DOCKER_DIR/"
  cp /tmp/rtorrent-docker/.dockerignore "$DOCKER_DIR/"
  cp /tmp/rtorrent-docker/.gitignore "$DOCKER_DIR/"

  rm -rf /tmp/rtorrent-docker
fi

# Remove vendored Go dependencies — fetched via 'go mod download' during build
rm -rf "$DOCKER_DIR/entrypoint/vendor"

# Override files with monorepo adaptations
cp -a "$OVERRIDES_DIR"/* "$DOCKER_DIR/"
