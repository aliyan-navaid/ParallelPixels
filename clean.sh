DIR1="filtered_images"
DIR2="images"

if [ -d "$DIR1" ]; then
  find "$DIR1" -mindepth 1 -delete
fi

if [ -d "$DIR2" ]; then
  find "$DIR2" -mindepth 1 -delete
fi