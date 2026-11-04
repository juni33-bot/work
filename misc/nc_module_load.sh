#!/bin/sh

if [ ! $APP_PATH ]; then
    APP_PATH=.
fi

if [ ! $MODULE_INI ]; then
    MODULE_INI=${APP_PATH}/nc_module.ini
fi

if [ ! $MODULE_PATH ]; then
    MODULE_PATH=${APP_PATH}/../modules
fi

read_ini() {
    grep -v "^;" $MODULE_INI | grep ^$1 | awk -F'=' '{print $2}' | awk '{print $1}'
}

CIS_0=$(read_ini "image_sensor_v0")
CIS_1=$(read_ini "image_sensor_v1")
SER_0=$(read_ini "serializer_v0")
SER_1=$(read_ini "serializer_v1")
DES_0=$(read_ini "deserializer_v0")
DES_1=$(read_ini "deserializer_v1")
TOTALCH_0=$(read_ini "totalch_v0")
TOTALCH_1=$(read_ini "totalch_v1")
SENSOR_TYPE_0=$(read_ini "sensor_type_v0")
SENSOR_TYPE_1=$(read_ini "sensor_type_v1")
OUTPUT_MODE_0=$(read_ini "output_mode_v0")
OUTPUT_MODE_1=$(read_ini "output_mode_v1")

print_parameter() {
    echo " MODULE PARAMETER (config:"$MODULE_INI")"
    echo "------------------------------------------------------------"
    echo "vision0 vision1" | awk '{ printf "                    %-20s %-20s\n", $1, $2}'
    echo "------------------------------------------------------------"
    echo "channel "$TOTALCH_0" "$TOTALCH_1 | awk '{ printf " %-19s%-20s%-20s\n", $1, $2, $3}'
    echo "image_sensor "$CIS_0" "$CIS_1 | awk '{ printf " %-19s%-20s%-20s\n", $1, $2, $3}'
    echo "serializer "$SER_0" "$SER_1 | awk '{ printf " %-19s%-20s%-20s\n", $1, $2, $3}'
    echo "deserializer "$DES_0" "$DES_1 | awk '{ printf " %-19s%-20s%-20s\n", $1, $2, $3}'
    echo "sensor_type "$SENSOR_TYPE_0" "$SENSOR_TYPE_1 | awk '{ printf " %-19s%-20s%-20s\n", $1, $2, $3}'
    echo "output_mode "$OUTPUT_MODE_0" "$OUTPUT_MODE_1 | awk '{ printf " %-19s%-20s%-20s\n", $1, $2, $3}'
    echo "------------------------------------------------------------"
}
print_parameter

insmod ${MODULE_PATH}/nc_camera_drv.ko totalch_0=$TOTALCH_0 cis_0=$CIS_0 ser_0=$SER_0 des_0=$DES_0 totalch_1=$TOTALCH_1 cis_1=$CIS_1 ser_1=$SER_1 des_1=$DES_1
insmod ${MODULE_PATH}/nc_platform_drv.ko sensor_type_0=$SENSOR_TYPE_0 output_mode_0=$OUTPUT_MODE_0 sensor_type_1=$SENSOR_TYPE_1 output_mode_1=$OUTPUT_MODE_1

exit 0
