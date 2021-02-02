#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
#
# Authors: Alexander Jung <a.jung@lancs.ac.uk>
#
# Copyright (c) 2021, Lancaster University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

set -o pipefail # trace ERR through pipes
set -o errtrace # trace ERR through 'time command' and other functions
set -o errexit  # exit the script if any statement returns a non-true return val

HELP=n

# Influential environmental variables
KRAFT=${KRAFT:-kraft}
EDITOR=${EDITOR:-nano}
UK_WORKDIR=${UK_WORKDIR:-$(pwd)}
UK_VERSION=${UK_VERSION:-}
UK_SUBVERSION=${UK_SUBVERSION:-}
UK_EXTRAVERSION=${UK_EXTRAVERSION:-}
UK_CODENAME=${UK_CODENAME:-}
GIT_NAME=${GIT_NAME:-}
GIT_EMAIL=${GIT_EMAIL:-}

export EDITOR=$EDITOR

# Program arguments
BUMP_MAJOR=0
BUMP_MINOR=0
BUMP_EXTRA=0
CODENAME=""
NEXT_VERSION=""
RELEASE_NOTE=""
APPEND_EXTRA=""
TAG_PREFIX="RELEASE-"
NO_COMMIT=n
NO_TAG=n
NO_MERGE=n
NO_PUSH=n
NO_PULL=n
VERBOSE=n
DRY_RUN=n
GIT_SIGNOFF=n
GIT_EDIT=n
SHOW_HELP=n
REPOS=()

CODENAMES=()
CODENAMES+=(2,Titan)
CODENAMES+=(3,Ipaetus)
CODENAMES+=(4,Rhea)
CODENAMES+=(5,Tethys)
CODENAMES+=(6,Dione)
CODENAMES+=(7,Mimas)
CODENAMES+=(8,Enceladus)
CODENAMES+=(9,Hyperion)
CODENAMES+=(10,Phoebe)
CODENAMES+=(11,Janus)
CODENAMES+=(12,Epimetheus)
CODENAMES+=(13,Atlas)
CODENAMES+=(14,Prometheus)
CODENAMES+=(15,Pandora)
CODENAMES+=(16,Telesto)
CODENAMES+=(17,Calypso)
CODENAMES+=(18,Helene)
CODENAMES+=(19,Pan)
CODENAMES+=(20,Kiviuq)
CODENAMES+=(21,Ijiraq)
CODENAMES+=(22,Paaliaq)
CODENAMES+=(23,Skathi)
CODENAMES+=(24,Albiorix)
CODENAMES+=(25,Erriapus)
CODENAMES+=(26,Siarnaq)
CODENAMES+=(27,Tarvos)
CODENAMES+=(28,Mundifari)
CODENAMES+=(29,Suttungr)
CODENAMES+=(30,Thrymr)
CODENAMES+=(31,Ymir)
CODENAMES+=(32,Narvi)
CODENAMES+=(33,Methone)
CODENAMES+=(34,Pallene)
CODENAMES+=(35,Polydeuces)
CODENAMES+=(36,Bebhionn)
CODENAMES+=(37,Hyrrokkin)
CODENAMES+=(38,Bergelmir)
CODENAMES+=(39,Hati)
CODENAMES+=(40,Farbauti)
CODENAMES+=(41,Bestla)
CODENAMES+=(42,Magrathea) 
CODENAMES+=(43,Aegir)
CODENAMES+=(44,Fenrir)
CODENAMES+=(45,Fornjot)
CODENAMES+=(46,Paphnis)
CODENAMES+=(47,Skoll)
CODENAMES+=(48,Greip)
CODENAMES+=(49,Jarnsaxa)
CODENAMES+=(50,Kari)
CODENAMES+=(51,Loge)
CODENAMES+=(52,Surtur)
CODENAMES+=(53,Anthe)
CODENAMES+=(54,Tarqeq)
CODENAMES+=(55,Aegaeon)
# What now?

_help() {
    cat <<EOF
$0 - Generate a release on a Unikraft repository.

Usage:
  $0 [OPTIONS] [REPO...]

Options:
  -M --major N            Bump current major version, e.g. (vX+N.0.0)
                          When unset, N=1.
  -m --minor N            Bump current minor version, e.g. (v0.X+N.0)
                          When unset, N=1.
  -x --extra N            Bump current extra version, e.g. (v0.0.X+N)
                          When unset, N=1.
  -a --append             Append to the extra version, e.g. (v0.0.0-test)
  -v --version VERSION    Set full custom version, e.g. (0.4.0-alpha.0)
  -c --codename           Set a new codename for the release.
  -f --release-note FILE  Specify a file to a note to use as a commit
                          message for the release.  When no release not is
                          specified, \$EDITOR will prompt.
  -p --prefix PREFIX      Prefix tags with string (default: "$TAG_PREFIX")
     --author             Commit author name.
     --email              Commit author email.
  -s --signoff            Sign off Git commits.
  -e --edit               Edit the commit message before committing.
     --no-pull            Do not pull updates from kraft.
     --no-commit          Do not commit this release to the repo.
     --no-tag             Do not tag the repository with the specified version.
     --no-merge           Do not merge staging into stable.
     --no-push            Do not push changes to remote repository.
  -D --dry-run            Run this script in dry run mode.
  -V --verbose            Be verbose.
  -h --help               Show this help menu.

Influential environmental variables:
  EDITOR                  Program to open for authoring a release note.
  KRAFT                   Location of kraft program.
  KRAFTRC                 Path to the .kraftrc file.
  UK_WORKDIR              Path to Unikraft repository.
  UK_CACHEDIR             Path to where kraft saves its cache.
  UK_VERSION              Exact major version to use (vX.0.0).
  UK_SUBVERSION           Exact minor version to use (v0.X.0).
  UK_EXTRAVERSION         Exact extra version to use (v0.0.X).
  UK_CODENAME             Codename for release.
  UK_KRAFT_GITHUB_TOKEN   Personal access token for kraft's access to GitHub.

Help:
  Versioning semantics are defined by https://semver.org/spec/v2.0.0.html

  For full guidelines on generating a release for Unikraft, please
  refer to the documentation on the main repository:
  https://github.com/unikraft/unikraft.git
EOF
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    -M|--major)
      BUMP_MAJOR=1;;
    -m|--minor)
      BUMP_MINOR=1;;
    -x|--extra)
      BUMP_EXTRA=1;;
    -a|--append)
      APPEND_EXTRA="$2"; shift;;
    -v|--version)
      NEXT_VERSION="$2"; shift;;
    -c|--codename)
      UK_CODENAME="$2"; shift;;
    -f|--release-note)
      RELEASE_NOTE="$2"; shift;;
    -p|--prefix)
      TAG_PREFIX="$2"; shift;;
    --author)
      GIT_NAME="$2"; shift;;
    --email)
      GIT_EMAIL="$2"; shift;;
    -s|--signoff)
      GIT_SIGNOFF=y;;
    -e|--edit)
      GIT_EDIT=y;;
    --no-pull)
      NO_PULL=y;;
    --no-commit)
      NO_COMMIT=y;;
    --no-merge)
      NO_MERGE=y;;
    --no-push)
      NO_PUSH=y;;
    --no-tag)
      NO_TAG=y;;
    -V|--verbose)
      VERBOSE=y;;
    -D|--dry-run)
      DRY_RUN=y;;
    -h|--help)
      SHOW_HELP=y;;
    *)
      REPOS+=("$1");;
  esac
  shift
done

if [[ $SHOW_HELP == 'y' ]]; then
  _help
  exit 0
fi

# Logger tools
NC='\033[0m'
RED='\033[0;31m'
log_err() { echo -e "[${RED}ERR${NC}] $1"; }
GRN='\033[0;32m'
log_inf() { echo -e "[${GRN}INF${NC}] $1"; }
YLO='\033[0;33m'
log_dbg() { if [[ $VERBOSE == "y" ]]; then echo -e "[${YLO}DBG${NC}] $1"; fi; }

# Sanity checks
# Check if all necessary tools to run ukbench are installed
function _check_tools {
  local MISSING=
  local COMMANDS=(
    $KRAFT
    readlink
    git
    jq
  )

  log_dbg "Checking for missing tools..."

  for COMMAND in "${COMMANDS[@]}"; do
    if [[ $VERBOSE == 'y' ]]; then
      printf '      * %-15s' "$COMMAND"
    fi
    if hash "$COMMAND" 2>/dev/null; then
      if [[ $VERBOSE == 'y' ]]; then
        echo OK
      fi
    else
      MISSING="$COMMAND ${MISSING}"
      if [[ $VERBOSE == 'y' ]]; then
        echo -e "\033[0;31mmissing\033[0m"
      fi
    fi
  done

  if [[ $MISSING != "" ]]; then
    log_err "Missing programs: $MISSING"
    exit 1
  fi
}; _check_tools

function maybe {
  local CMD="$@"

  log_inf "${CMD}"
  if [[ $DRY_RUN != 'y' ]]; then
    bash -c "${CMD}"
  fi
}

if [[ $RELEASE_NOTE != "" && ! -f $RELEASE_NOTE ]]; then
  log_err "Cannot find release note: ${RELEASE_NOTE}"
  exit 1
else
  RELEASE_NOTE=$(readlink -f ${RELEASE_NOTE})
fi

if [[ $GIT_NAME == "" ]]; then
  GIT_NAME=$(git config --get user.name)
fi

if [[ $GIT_NAME == "" ]]; then
  log_err "Missing committer name"
  exit 1
fi

if [[ $GIT_EMAIL == "" ]]; then
  GIT_EMAIL=$(git config --get user.email)
fi

if [[ $GIT_EMAIL == "" ]]; then
  log_err "Missing committer email"
  exit 1
fi

log_dbg "Committer......: ${GIT_NAME} <$GIT_EMAIL>"

if [[ $REPOS == "" ]]; then
  if [[ $NO_PULL != 'y' ]]; then
    log_dbg "Using kraft to determine repositories..."
    $KRAFT list update
  fi

  JSON=$(kraft list --json)

  if [[ $NO_PULL != 'y' ]]; then
    maybe "kraft list pull -D unikraft@staging"
  fi

  REPOS+=($(kraft list show --json unikraft | jq -r ".[].meta.localdir"))

  PLATS=$(echo $JSON | jq -r ".platforms")

  for PLAT in $(echo $PLATS | jq -r ".[] | @base64"); do
    _jq() {
      echo ${PLAT} | base64 --decode | jq -r "${1}"
    }

    LOCALDIR=$(_jq ".meta.localdir")

    if [[ $NO_PULL != 'y' ]]; then
      maybe "kraft list pull -D plat/$(_jq .meta.name)@staging"
    elif [[ ! -d $LOCALDIR ]]; then
      log_err "Cannot find: $LOCALDIR"
    fi

    REPOS+=($LOCALDIR)
  done

  APPS=$(echo $JSON | jq -r ".applications")

  for APP in $(echo $APPS | jq -r ".[] | @base64"); do
    _jq() {
      echo ${APP} | base64 --decode | jq -r "${1}"
    }

    LOCALDIR=$(_jq ".meta.localdir")

    if [[ $NO_PULL != 'y' ]]; then
      maybe "kraft list pull -D app/$(_jq .meta.name)@staging"
    elif [[ ! -d $LOCALDIR ]]; then
      log_err "Cannot find: $LOCALDIR"
    fi

    REPOS+=($LOCALDIR)
  done

  LIBS=$(echo $JSON | jq -r ".libraries")

  for LIB in $(echo $LIBS | jq -r ".[] | @base64"); do
    _jq() {
      echo ${LIB} | base64 --decode | jq -r "${1}"
    }

    LOCALDIR=$(_jq ".meta.localdir")

    if [[ $NO_PULL != 'y' ]]; then
      maybe "kraft list pull -D lib/$(_jq .meta.name)@staging"
    elif [[ ! -d $LOCALDIR ]]; then
      log_err "Cannot find: $LOCALDIR"
    fi

    REPOS+=($LOCALDIR)
  done
fi

# Go through each repository and bump
for REPO in "${REPOS[@]}"; do
  if [[ -d $REPO ]]; then
    REPO=$(readlink -f $REPO)
  elif [[ -d $UK_WORKDIR/$REPO ]]; then
    REPO=$(readlink -f $UK_WORKDIR/$REPO)
  else
    log_err "Cannot find path: $REPO"
    continue
  fi

  if [[ ! -d $REPO/.git ]]; then
    log_err "Path is not a Git repository: $REPO"
    continue
  fi

  log_dbg "Parsing repository: ${REPO}"

  if [[ $(git -C ${REPO} branch -ar | grep staging) == "" ]]; then
    log_err "${REPO} does not have remote staging branch!"
    continue
  elif [[ $NO_PULL != 'y' ]]; then
    maybe "git -C ${REPO} pull origin staging"
  fi

  if [[ $(git -C ${REPO} branch -ar | grep stable) == "" ]]; then
    log_err "${REPO} does not have remote stable branch!"
    continue
  elif [[ $NO_PULL != 'y' ]]; then
    maybe "git -C ${REPO} pull origin stable"
  fi

  if [ $(ls -A ${REPO}) == ".git" ]; then
    log_err "${REPO} is empty!"
    continue
  fi

  # Fix problems that may occur with branch origin/name
  maybe "git -C ${REPO} checkout origin/staging"
  maybe "git -C ${REPO} checkout staging || git -C ${REPO} checkout -b staging"

  if [[ $NEXT_VERSION == "" ]]; then
    git -C $REPO update-index -q --refresh

    CURR=$(git -C $REPO for-each-ref --sort=-taggerdate --format '%(tag)' refs/tags |\
           sed "s/${TAG_PREFIX}//g" |\
           head -n 1)

    # Break version down
    IFS='.' read -r -a VPARTS <<< $CURR
    CURR_MAJOR="${VPARTS[0]}"
    CURR_MINOR="${VPARTS[1]}"
    CURR_EXTRA="${VPARTS[2]}"
    if [[ $CURR_EXTRA == "" ]]; then
      CURR_EXTRA=0
    fi

    CURR_VERSION="${CURR_MAJOR}.${CURR_MINOR}.${CURR_EXTRA}"
    log_inf "Current version: v${CURR_VERSION}"

    NEXT_MAJOR=$CURR_MAJOR
    NEXT_MINOR=$CURR_MINOR
    NEXT_EXTRA=$CURR_EXTRA

    # Bump versions appropriately
    if [[ $BUMP_MAJOR > 0 ]]; then
      NEXT_MAJOR=$(($CURR_MAJOR+$BUMP_MAJOR))
      NEXT_MINOR=0
      NEXT_EXTRA=0
    elif [[ $BUMP_MINOR > 0 ]]; then
      NEXT_MINOR=$(($CURR_MINOR+$BUMP_MINOR))
      NEXT_EXTRA=0
    elif [[ $BUMP_EXTRA > 0 ]]; then
      NEXT_EXTRA=$(($CURR_EXTRA+$BUMP_EXTRA))
    fi
  else
    # Break version down if -v is specified
    IFS='.' read -r -a VPARTS <<< $NEXT_VERSION
    NEXT_MAJOR="${VPARTS[0]}"
    NEXT_MINOR="${VPARTS[1]}"
    NEXT_EXTRA="${VPARTS[2]}"
    if [[ $NEXT_EXTRA == "" ]]; then
      NEXT_EXTRA=0
    fi
  fi

  if [[ $UK_VERSION != "" ]]; then
    NEXT_MAJOR=$UK_VERSION
  fi
  if [[ $UK_SUBVERSION != "" ]]; then
    NEXT_MINOR=$UK_SUBVERSION
  fi
  if [[ $UK_EXTRAVERSION != "" ]]; then
    NEXT_EXTRA=$UK_EXTRAVERSION
  fi

  if [[ $APPEND_EXTRA != "" ]]; then
    NEXT_EXTRA="${NEXT_EXTRA}-${APPEND_EXTRA}"
  fi

  NEXT_VERSION="${NEXT_MAJOR}.${NEXT_MINOR}.${NEXT_EXTRA}"

  # Remove v prefix from version
  if [[ ${NEXT_VERSION:0:1} == "v" ]]; then
    NEXT_VERSION=${NEXT_VERSION:1}
  fi    

  if [[ $NEXT_VERSION == $CURR_VERSION ]]; then
    log_err "Next version cannot be same as current version: ${CURR_VERSION}"
    exit 1
  fi

  log_inf "Next version...: v${NEXT_VERSION}"

  if [[ $UK_CODENAME == "" ]]; then
    for NAME in "${CODENAMES[@]}"; do
      IFS=',' read -r -a CPARTS <<< $NAME
      if [[ "${CPARTS[0]}" == $NEXT_MINOR ]]; then
        UK_CODENAME="${CPARTS[1]}"
        break
      fi
    done
  fi

  if [[ $UK_CODENAME == "" ]]; then
    log_err "No codename specified!"
    exit 1
  fi

  log_inf "Codename.......: ${UK_CODENAME}"

  RELEASE_HEADING="Release: v${NEXT_VERSION} ${UK_CODENAME}"

  if [[ $GIT_NAME != "" ]]; then
    maybe "git -C ${REPO} config user.name ${GIT_NAME}"
    if [[ $GIT_EMAIL != "" ]]; then
    maybe "git -C ${REPO} config user.email ${GIT_EMAIL}"
    fi
  fi

  # The core has special commit-moons
  if [[ -f $REPO/version.mk ]]; then
    if [[ $DRY_RUN != 'y' ]]; then
      cat <<EOF > $REPO/version.mk
UK_VERSION = ${NEXT_MAJOR}
UK_SUBVERSION = ${NEXT_MINOR}
UK_EXTRAVERSION = ${NEXT_EXTRA}
UK_CODENAME = ${UK_CODENAME}
# https://en.wikipedia.org/wiki/Moons_of_Saturn (by discovery year)
EOF
    fi

    if [[ $VERBOSE == 'y' ]]; then
      maybe "git -C ${REPO} diff ${REPO}/version.mk"
    fi

    if [[ $NO_COMMIT != 'y' ]]; then
      maybe "git -C ${REPO} add ${REPO}/version.mk"

      COMMIT_EXTRA=()
      if [[ $DRY_RUN == 'y' ]]; then
        COMMIT_EXTRA+=("--dry-run")
      fi

      if [[ $GIT_SIGNOFF == 'y' ]]; then
        COMMIT_EXTRA+=("-s")
      fi
      
      if [[ $GIT_EDIT == 'y' ]]; then
        COMMIT_EXTRA+=("-e")
      fi

      if [[ $VERBOSE == 'y' ]]; then
        COMMIT_EXTRA+=("-v")
      fi

      if [[ -f $RELEASE_NOTE ]]; then
        COMMIT_EXTRA="${COMMIT_EXTRA[@]} -F ${RELEASE_NOTE}"
      else
        COMMIT_EXTRA="${COMMIT_EXTRA[@]} -m \"${RELEASE_HEADING}\""
      fi

      log_inf "git -C ${REPO} commit ${COMMIT_EXTRA[@]}"
      bash -c "git -C ${REPO} commit ${COMMIT_EXTRA[@]}"
    fi
  fi

  if [[ $NO_MERGE != 'y' ]]; then
    maybe "git -C ${REPO} checkout origin/stable"
    maybe "git -C ${REPO} checkout stable || git -C ${REPO} checkout -b stable"
    maybe "git -C ${REPO} merge staging"
  fi

  if [[ $NO_TAG != 'y' ]]; then
    maybe "git -C ${REPO} tag -a ${TAG_PREFIX}${NEXT_VERSION} -m '${RELEASE_HEADING}'"
  fi

  if [[ $NO_PUSH != 'y' ]]; then
    maybe "git -C ${REPO} push origin stable"
  fi
done
