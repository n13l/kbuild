find . -type f -name "*.h" -exec sed -i "s/include <posix\//include <unix\//g" {} +
