#! /bin/sh

OSTYPE=$(uname)

if [ "${OSTYPE}" != "Darwin" ]; then
    echo "[Error] macOS install dependencies script can be run on Darwin-type OS only."
    exit 1
fi

HAS_BREW=$(type brew 2>/dev/null)

if [ "${HAS_BREW}" = "" ]; then
    echo "[Error] Please install Homebrew (https://www.brew.sh/) to build this plugin on macOS."
    exit 1
fi

# OBS-VNC deps
echo "=> Updating Homebrew.."
brew update >/dev/null

echo "[=> Checking installed Homebrew formulas.."
BREW_PACKAGES=$(brew list --formula)
# dependencies for libvncserver
BREW_DEPENDENCIES="lzo"

for DEPENDENCY in ${BREW_DEPENDENCIES}; do
    if echo "${BREW_PACKAGES}" | grep -q "^${DEPENDENCY}\$"; then
        echo "=> Upgrading OBS-VNC dependency '${DEPENDENCY}'.."
        brew upgrade ${DEPENDENCY} 2>/dev/null
    else
        echo "=> Installing OBS-VNC dependency '${DEPENDENCY}'.."
        brew install ${DEPENDENCY} 2>/dev/null
    fi
done
