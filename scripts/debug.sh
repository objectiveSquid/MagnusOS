unset GTK_PATH
unset GIO_MODULE_DIR

FILE_DIRECTORY=$(dirname $(realpath $0))

bochs -f $FILE_DIRECTORY/bochs_config
