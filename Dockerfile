# Build environment for DScraft (Nintendo DS homebrew, devkitARM + libnds)
# Pinned to the last release before libnds 2.0/calico: the game relies on
# libnds 1.x APIs and on libfat 1.1.x internals (_FAT_open_r, FILE_STRUCT).
FROM devkitpro/devkitarm:20240918
WORKDIR /dscraft
CMD ["make"]
