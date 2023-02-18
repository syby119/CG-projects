function(hardlink_shaders target directory shaders)
    add_custom_command(
        TARGET ${target}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        make_directory ${directory}
        COMMENT "make directory ${directory}"
    )

    set(first True)
    foreach (shader ${${shaders}})
        get_filename_component(name ${shader} NAME)

        if (first)
            set(cmd_comment "create_hardlink shaders to ${directory}\n")
            set(first False)
        else()
            set(cmd_comment "")
        endif()

        add_custom_command(
            TARGET ${target}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E
            create_hardlink ${shader} ${directory}/${name}
            COMMENT ${cmd_comment}
        )
    endforeach(shader)
endfunction()