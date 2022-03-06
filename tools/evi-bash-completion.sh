# evi(1) completion                                        -*- shell-script -*-

_evi_autocomplete()
{
    local cur prev words cword split
    _init_completion -s || return

    case $prev in
        -h|--help|--usage|-V|--version)
            return
            ;;
        -l|--link)
            _filedir
            return
            ;;
        -i|--include)
            _filedir '@'
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
complete -F _evi_autocomplete evi