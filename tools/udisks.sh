#!/bin/sh
# Montar e desmontar discos removíveis (cartão do stick, pendrive) SEM sudo, pelo udisks2.
# Para ser lido por outro script com `. tools/udisks.sh`; não faz nada sozinho.
#
# O polkit libera `filesystem-mount` sem senha na sessão ativa. Desmontar o que outro usuário ou o
# root montou (`filesystem-unmount-others`) pede administrador; a regra temporária
# tools/polkit/60-udisks-temp.rules tira esse pedido até a data escrita nela. Regra completa no
# CLAUDE.md ("Montar e desmontar dispositivos"). Nunca `sudo mount`/`sudo umount` aqui.

# Partições que ESTE script montou (para desmontar só elas no fim)
UDISKS_MOUNTED=

# Monta toda partição removível ainda desmontada. Sem udisksctl ou lsblk, não faz nada.
udisks_mount_removable() {
    command -v udisksctl >/dev/null 2>&1 || return 0
    command -v lsblk >/dev/null 2>&1 || return 0
    # NAME TYPE RM [MOUNTPOINT]: só entram partições removíveis sem ponto de montagem (3 campos)
    for dev in $(lsblk -rpno NAME,TYPE,RM,MOUNTPOINT | awk '$2 == "part" && $3 == 1 && NF == 3 { print $1 }'); do
        echo ">> montando $dev (udisksctl)"
        if udisksctl mount -b "$dev"; then
            UDISKS_MOUNTED="$UDISKS_MOUNTED $dev"
        else
            echo "AVISO: não consegui montar $dev. Monte-o à mão ou veja a regra temporária em" >&2
            echo "  tools/polkit/60-udisks-temp.rules (make udisks-rule)." >&2
        fi
    done
}

# Desmonta o que udisks_mount_removable montou. Se faltar permissão, mostra o comando com pkexec.
udisks_unmount() {
    for dev in $UDISKS_MOUNTED; do
        echo ">> desmontando $dev (udisksctl)"
        if ! udisksctl unmount -b "$dev"; then
            echo "AVISO: não desmontei $dev. Tente: pkexec udisksctl unmount -b $dev" >&2
            echo "  (ou instale a regra temporária: make udisks-rule)" >&2
        fi
    done
    UDISKS_MOUNTED=
}
