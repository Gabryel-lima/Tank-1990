# Este arquivo foi gerado por um assistente de criação de projetos do Qt Creator.
# O código abaixo gerencia o deploy para Android e Maemo, assim como a cópia
# dos dados da aplicação para diretórios de build "shadow" em desktops.
# Recomenda-se não modificar este arquivo, pois versões mais recentes do Qt Creator
# podem oferecer uma versão atualizada dele.

# Função principal de deploy chamada pelo qmake
defineTest(qtcAddDeployment) {
    # Para cada pasta de deploy definida em DEPLOYMENTFOLDERS
    for(deploymentfolder, DEPLOYMENTFOLDERS) {
        item = item$${deploymentfolder}
        # Qt5 ou superior usa .files, Qt4 usa .sources
        greaterThan(QT_MAJOR_VERSION, 4) {
            itemsources = $${item}.files
        } else {
            itemsources = $${item}.sources
        }
        # Define os arquivos de origem e destino para deploy
        $$itemsources = $$eval($${deploymentfolder}.source)
        itempath = $${item}.path
        $$itempath= $$eval($${deploymentfolder}.target)
        export($$itemsources)
        export($$itempath)
        DEPLOYMENT += $$item
    }

    MAINPROFILEPWD = $$PWD

    # Deploy específico para Android sem SDK
    android-no-sdk {
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            item = item$${deploymentfolder}
            itemfiles = $${item}.files
            $$itemfiles = $$eval($${deploymentfolder}.source)
            itempath = $${item}.path
            $$itempath = /data/user/qt/$$eval($${deploymentfolder}.target)
            export($$itemfiles)
            export($$itempath)
            INSTALLS += $$item
        }

        target.path = /data/user/qt

        export(target.path)
        INSTALLS += target
    } else:android {
        # Deploy para Android com SDK
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            item = item$${deploymentfolder}
            itemfiles = $${item}.files
            $$itemfiles = $$eval($${deploymentfolder}.source)
            itempath = $${item}.path
            $$itempath = /assets/$$eval($${deploymentfolder}.target)
            export($$itemfiles)
            export($$itempath)
            INSTALLS += $$item
        }

        # Define o caminho de instalação conforme arquitetura
        x86 {
            target.path = /libs/x86
        } else: armeabi-v7a {
            target.path = /libs/armeabi-v7a
        } else {
            target.path = /libs/armeabi
        }

        export(target.path)
        INSTALLS += target
    } else:win32 {
        # Deploy para Windows: cópia dos arquivos usando comandos de sistema
        copyCommand =
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            source = $$MAINPROFILEPWD/$$eval($${deploymentfolder}.source)
            source = $$replace(source, /, \\)
            sourcePathSegments = $$split(source, \\)
            target = $$OUT_PWD/$$eval($${deploymentfolder}.target)/$$last(sourcePathSegments)
            target = $$replace(target, /, \\)
            target ~= s,\\\\\\.?\\\\,\\,
            !isEqual(source,$$target) {
                !isEmpty(copyCommand):copyCommand += &&
                isEqual(QMAKE_DIR_SEP, \\) {
                    # Comando para copiar diretórios no Windows
                    copyCommand += $(COPY_DIR) \"$$source\" \"$$target\"
                } else {
                    # Comando para copiar diretórios em ambientes Unix-like no Windows
                    source = $$replace(source, \\\\, /)
                    target = $$OUT_PWD/$$eval($${deploymentfolder}.target)
                    target = $$replace(target, \\\\, /)
                    copyCommand += test -d \"$$target\" || mkdir -p \"$$target\" && cp -r \"$$source\" \"$$target\"
                }
            }
        }
        !isEmpty(copyCommand) {
            copyCommand = @echo Copiando dados da aplicação... && $$copyCommand
            copydeploymentfolders.commands = $$copyCommand
            first.depends = $(first) copydeploymentfolders
            export(first.depends)
            export(copydeploymentfolders.commands)
            QMAKE_EXTRA_TARGETS += first copydeploymentfolders
        }
    } else:ios {
        # Deploy para iOS: cópia dos arquivos para o bundle assinado
        copyCommand =
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            source = $$MAINPROFILEPWD/$$eval($${deploymentfolder}.source)
            source = $$replace(source, \\\\, /)
            target = $CODESIGNING_FOLDER_PATH/$$eval($${deploymentfolder}.target)
            target = $$replace(target, \\\\, /)
            sourcePathSegments = $$split(source, /)
            targetFullPath = $$target/$$last(sourcePathSegments)
            targetFullPath ~= s,/\\.?/,/,
            !isEqual(source,$$targetFullPath) {
                !isEmpty(copyCommand):copyCommand += &&
                copyCommand += mkdir -p \"$$target\"
                copyCommand += && cp -r \"$$source\" \"$$target\"
            }
        }
        !isEmpty(copyCommand) {
            copyCommand = echo Copiando dados da aplicação... && $$copyCommand
            !isEmpty(QMAKE_POST_LINK): QMAKE_POST_LINK += ";"
            QMAKE_POST_LINK += "$$copyCommand"
            export(QMAKE_POST_LINK)
        }
    } else:unix {
        # Deploy para sistemas Unix (Linux, Mac, etc)
        maemo5 {
            # Configuração para Maemo5
            desktopfile.files = $${TARGET}.desktop
            desktopfile.path = /usr/share/applications/hildon
            icon.files = $${TARGET}64.png
            icon.path = /usr/share/icons/hicolor/64x64/apps
        } else:!isEmpty(MEEGO_VERSION_MAJOR) {
            # Configuração para MeeGo
            desktopfile.files = $${TARGET}_harmattan.desktop
            desktopfile.path = /usr/share/applications
            icon.files = $${TARGET}80.png
            icon.path = /usr/share/icons/hicolor/80x80/apps
        } else { # Assumido como Desktop Unix padrão
            copyCommand =
            for(deploymentfolder, DEPLOYMENTFOLDERS) {
                source = $$MAINPROFILEPWD/$$eval($${deploymentfolder}.source)
                source = $$replace(source, \\\\, /)
                macx {
                    # Para MacOS, copia para o bundle .app
                    target = $$OUT_PWD/$${TARGET}.app/Contents/Resources/$$eval($${deploymentfolder}.target)
                } else {
                    target = $$OUT_PWD/$$eval($${deploymentfolder}.target)
                }
                target = $$replace(target, \\\\, /)
                sourcePathSegments = $$split(source, /)
                targetFullPath = $$target/$$last(sourcePathSegments)
                targetFullPath ~= s,/\\.?/,/,
                !isEqual(source,$$targetFullPath) {
                    !isEmpty(copyCommand):copyCommand += &&
                    copyCommand += $(MKDIR) \"$$target\"
                    copyCommand += && $(COPY_DIR) \"$$source\" \"$$target\"
                }
            }
            !isEmpty(copyCommand) {
                copyCommand = @echo Copiando dados da aplicação... && $$copyCommand
                copydeploymentfolders.commands = $$copyCommand
                first.depends = $(first) copydeploymentfolders
                export(first.depends)
                export(copydeploymentfolders.commands)
                QMAKE_EXTRA_TARGETS += first copydeploymentfolders
            }
        }
        # Define o prefixo de instalação
        !isEmpty(target.path) {
            installPrefix = $${target.path}
        } else {
            installPrefix = /opt/$${TARGET}
        }
        # Adiciona cada item de deploy ao INSTALLS
        for(deploymentfolder, DEPLOYMENTFOLDERS) {
            item = item$${deploymentfolder}
            itemfiles = $${item}.files
            $$itemfiles = $$eval($${deploymentfolder}.source)
            itempath = $${item}.path
            $$itempath = $${installPrefix}/$$eval($${deploymentfolder}.target)
            export($$itemfiles)
            export($$itempath)
            INSTALLS += $$item
        }

        # Exporta arquivos de desktop e ícone, se existirem
        !isEmpty(desktopfile.path) {
            export(icon.files)
            export(icon.path)
            export(desktopfile.files)
            export(desktopfile.path)
            INSTALLS += icon desktopfile
        }

        # Define o caminho padrão do executável, se não definido
        isEmpty(target.path) {
            target.path = $${installPrefix}/bin
            export(target.path)
        }
        INSTALLS += target
    }

    # Exporta variáveis importantes para o qmake
    export (ICON)
    export (INSTALLS)
    export (DEPLOYMENT)
    export (LIBS)
    export (QMAKE_EXTRA_TARGETS)
}
