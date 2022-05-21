#!/usr/bin/env bash

# bash tab-completion

# This is a per-library function definition, used in conjunction with the
# top-level entry point in ign-tools.

function _gz_service
{
  if [[ ${COMP_WORDS[COMP_CWORD]} == -* ]]; then
    # Specify options (-*) word list for this subcommand
    # NOTE: In Fortress+, remove --force-version and --versions. Add --help-all.
    # Update ../ign_TEST.cc accordingly.
    COMPREPLY=($(compgen -W "
      -h --help
      -v --version
      -s --service
      --reqtype
      --reptype
      --timeout
      -l --list
      -i --info
      -r --req
      --force-version
      --versions
      " -- "${COMP_WORDS[COMP_CWORD]}" ))
    return
  else
    # Just use bash default auto-complete, because we never have two
    # subcommands in the same line. If that is ever needed, change here to
    # detect subsequent subcommands
    COMPREPLY=($(compgen -o default -- "${COMP_WORDS[COMP_CWORD]}"))
    return
  fi
}

function _gz_topic
{
  if [[ ${COMP_WORDS[COMP_CWORD]} == -* ]]; then
    # Specify options (-*) word list for this subcommand
    # NOTE: In Fortress+, remove --force-version and --versions. Add
    # `-v --version` and --json-output. Update ../ign_TEST.cc accordingly.
    COMPREPLY=($(compgen -W "
      -h --help
      -v --version
      -t --topic
      -m --msgtype
      -d --duration
      -n --num
      -l --list
      -i --info
      -e --echo
      -p --pub
      --force-version
      --versions
      " -- "${COMP_WORDS[COMP_CWORD]}" ))
    return
  else
    # Just use bash default auto-complete, because we never have two
    # subcommands in the same line. If that is ever needed, change here to
    # detect subsequent subcommands
    COMPREPLY=($(compgen -o default -- "${COMP_WORDS[COMP_CWORD]}"))
    return
  fi
}
