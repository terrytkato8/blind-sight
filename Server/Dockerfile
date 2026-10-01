# Blind Sight dedicated server.
#
# Expects a cooked Linux server build at ./LinuxServer, produced by:
#   RunUAT.sh BuildCookRun -project=<abs path>/BlindSight.uproject \
#     -noP4 -platform=Linux -server -serverplatform=Linux -noclient \
#     -cook -build -stage -pak -archive -archivedirectory=<out>
#
# Then: docker build -f Server/Dockerfile -t blindsight-server:0.1 .
FROM ubuntu:22.04

# UE Linux server needs very little: libc, SSL for the HTTPS backend calls, and ICU.
RUN apt-get update && apt-get install -y --no-install-recommends \
      ca-certificates libssl3 libicu70 xdg-user-dirs curl \
 && rm -rf /var/lib/apt/lists/*

RUN useradd -m -u 10001 ue
WORKDIR /home/ue/server

COPY --chown=ue:ue LinuxServer/ ./
COPY --chown=ue:ue Server/entrypoint.sh /home/ue/entrypoint.sh
RUN chmod +x /home/ue/entrypoint.sh \
 && chmod +x ./BlindSightServer.sh 2>/dev/null || true

USER ue

# Game traffic. UE's default is 7777/udp; the orchestrator maps a public port in.
EXPOSE 7777/udp

ENV BS_PORT=7777 \
    BS_REGION=local \
    BS_MAP=L_Slice01 \
    BS_MODE=/Script/BlindSight.BSGameMode_Blackout

ENTRYPOINT ["/home/ue/entrypoint.sh"]
