FROM ghcr.io/wiiu-env/devkitppc:20231112

COPY --from=ghcr.io/wiiu-env/libnotifications:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmappedmemory:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20230719 /artifacts $DEVKITPRO

WORKDIR project
