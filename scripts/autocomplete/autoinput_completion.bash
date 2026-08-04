# autoinput bash completion

_autoinput_configs() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local configs=( $(ls configs/*.toml ../configs/*.toml ~/.autoinput/*.toml 2>/dev/null | xargs -n1 basename | sed 's/\.toml$//' | sort -u) )
    COMPREPLY=( $(compgen -W "${configs[*]}" -- "$cur") )
}

_autoinput() {
    local cur prev words cword
    _get_comp_words_by_ref -n : cur prev words cword

    local commands="run record config apps help"
    local global_opts="-h --help --examples -l --log --json"
    
    local log_levels="d debug i info w warn warning e error f fatal"
    local action_types="click c hold h"
    local mouse_buttons="left l right r middle m back forward"
    local common_keys="a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 esc escape space tab enter return backspace ins insert del delete home end pageup pgup pagedown pgdn up down left right capslock numlock scrolllock printscreen prtsc pause"
    
    # Simple check for command
    local command=""
    local i
    for ((i=1; i < cword; i++)); do
        if [[ " ${commands} " == *" ${words[i]} "* ]]; then
            command="${words[i]}"
            break
        fi
    done

    if [[ -z "$command" ]]; then
        if [[ "$cur" == -* ]]; then
            COMPREPLY=( $(compgen -W "${global_opts}" -- "$cur") )
        else
            COMPREPLY=( $(compgen -W "${commands}" -- "$cur") )
        fi
        return 0
    fi

    case "$command" in
        run)
            case "$prev" in
                -c|--config|-S|--save-config)
                    _autoinput_configs
                    return 0
                    ;;
                -t|--type)
                    COMPREPLY=( $(compgen -W "${action_types}" -- "$cur") )
                    return 0
                    ;;
                -b|--btn|--button)
                    COMPREPLY=( $(compgen -W "${mouse_buttons}" -- "$cur") )
                    return 0
                    ;;
                -k|--key|-s|--start|-e|--end)
                    COMPREPLY=( $(compgen -W "${common_keys}" -- "$cur") )
                    return 0
                    ;;
                --status-notification)
                    COMPREPLY=( $(compgen -W "off console desktop both" -- "$cur") )
                    return 0
                    ;;
            esac
            COMPREPLY=( $(compgen -W "-c --config -t --type -b --btn --button -k --key -s --start -e --end -a --app --application -B --blacklist -w --wait --press-wait --release-wait --status-notification -S --save-config" -- "$cur") )
            ;;
        record)
            case "$prev" in
                --start|--end|--play-start)
                    COMPREPLY=( $(compgen -W "${common_keys}" -- "$cur") )
                    return 0
                    ;;
            esac
            COMPREPLY=( $(compgen -W "--start --end --play-start --mouse-moves --mouse-sample --force" -- "$cur") )
            ;;
        config)
            local subcommands="list validate duplicate copy"
            local subcmd=""
            for ((i=1; i < cword; i++)); do
                if [[ " ${subcommands} " == *" ${words[i]} "* ]]; then
                    subcmd="${words[i]}"
                    break
                fi
            done

            if [[ -z "$subcmd" ]]; then
                COMPREPLY=( $(compgen -W "${subcommands}" -- "$cur") )
            else
                case "$subcmd" in
                    validate|duplicate|copy)
                        if [[ "$prev" == "$subcmd" ]]; then
                            _autoinput_configs
                            return 0
                        fi
                        ;;
                esac
                if [[ "$subcmd" == "duplicate" || "$subcmd" == "copy" ]]; then
                    COMPREPLY=( $(compgen -W "--force" -- "$cur") )
                fi
            fi
            ;;
        apps)
            COMPREPLY=( $(compgen -W "list" -- "$cur") )
            ;;
        help)
            COMPREPLY=( $(compgen -W "run record config apps" -- "$cur") )
            ;;
    esac
}

complete -F _autoinput autoinput
