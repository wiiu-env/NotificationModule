FROM wiiuenv/devkitppc:20221228

COPY --from=wiiuenv/libnotifications:20230126 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libmappedmemory:20220904 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20230108 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20230106 /artifacts $DEVKITPRO

WORKDIR project
