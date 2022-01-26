# evi(1) completion                                        -*- shell-script -*-

_evi()
{
    local cur prev words cword split
    _init_completion -s || return

    case $prev in
        -'?'|--help|--usage|-V|--version)
            return
            ;;
    esac

    $split && return

    if [[ $cur == -* ]]; then
        COMPREPLY=( $(compgen -W '$(_parse_help "$1" --help)' -- "$cur") )
        [[ $COMPREPLY == *= ]] && compopt -o nospace
        return
    fi

    _filedir '@(evi)'
} &&
complete -F _evi evi

# ex: filetype=sh