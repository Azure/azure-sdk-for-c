if (CONFIG_AZ)

zephyr_interface_library_named(az)

zephyr_include_directories(../sdk/inc)

zephyr_library()

# core
zephyr_library_sources(../sdk/src/azure/core/az_base64.c)
zephyr_library_sources(../sdk/src/azure/core/az_context.c)
zephyr_library_sources(../sdk/src/azure/core/az_http_pipeline.c)
zephyr_library_sources(../sdk/src/azure/core/az_http_policy.c)
zephyr_library_sources(../sdk/src/azure/core/az_http_policy_logging.c)
zephyr_library_sources(../sdk/src/azure/core/az_http_policy_retry.c)
zephyr_library_sources(../sdk/src/azure/core/az_http_request.c)
zephyr_library_sources(../sdk/src/azure/core/az_http_response.c)
zephyr_library_sources(../sdk/src/azure/core/az_json_reader.c)
zephyr_library_sources(../sdk/src/azure/core/az_json_token.c)
zephyr_library_sources(../sdk/src/azure/core/az_json_writer.c)
zephyr_library_sources(../sdk/src/azure/core/az_log.c)
zephyr_library_sources(../sdk/src/azure/core/az_precondition.c)
zephyr_library_sources(../sdk/src/azure/core/az_span.c)

# iot
zephyr_library_sources(../sdk/src/azure/iot/az_iot_common.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_c2d.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_commands.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_methods.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_properties.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_sas.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_telemetry.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_HUB ../sdk/src/azure/iot/az_iot_hub_client_twin.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_PROVISIONING ../sdk/src/azure/iot/az_iot_provisioning_client.c)
zephyr_library_sources_ifdef(CONFIG_AZ_IOT_PROVISIONING ../sdk/src/azure/iot/az_iot_provisioning_client_sas.c)

# platform
zephyr_library_sources(../sdk/src/azure/platform/az_zephyr.c)

zephyr_link_libraries(az)

endif(CONFIG_AZ)
