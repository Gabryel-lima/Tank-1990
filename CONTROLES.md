# Controles Bluetooth genéricos: investigação e arquitetura

Como um programa próprio recebe os botões, analógicos e gatilhos de um controle de videogame
**genérico, sem fio (Bluetooth)**, no Windows, no Linux, no macOS e no WSL2, e como o Tank 1990
implementa isso.

**Legenda de certeza** (usada em todo o documento):

| Marca | Significado |
|-------|-------------|
| **[F]** | Fato verificado nesta investigação, em fonte primária (código-fonte, arquivo de configuração, documentação oficial). A fonte está no fim. |
| **[B]** | Fato bem estabelecido (especificação ou comportamento conhecido), não reverificado em fonte primária nesta investigação. |
| **[I]** | Inferência a partir de fatos. |
| **[H]** | Hipótese ou caminho experimental: plausível, não testado aqui. |
| **[O]** | Opinião de projeto. |

---

## Resumo

- Um controle Bluetooth genérico quase sempre é um **dispositivo HID**. O formato dos dados (o
  *descritor de relatório* e os *relatórios*) é o mesmo no USB, no Bluetooth Clássico e no
  Bluetooth LE; muda só o transporte. O sistema operacional transforma os três num "dispositivo
  HID" igual, e daí para cima o transporte é invisível. **[B]**
- Por isso, **a camada certa para um programa próprio é a de HID já interpretado ou a de eventos
  de jogo, não a de Bluetooth**. Windows e Apple nem deixam um aplicativo falar com o serviço HID
  de um dispositivo Bluetooth diretamente. **[F]**
- A abstração multiplataforma mais completa é o **SDL** (SDL2 ou SDL3). Ele usa, em cada sistema, a
  melhor API disponível e ainda tem drivers próprios por cima de HID bruto (hidapi) para controles
  conhecidos. **[F]**
- No **WSL2**, o caminho robusto é **não levar o Bluetooth para o Linux**: ler o controle no
  Windows e encaminhar o estado para o WSL, onde ele vira um controle virtual. Repassar o adaptador
  Bluetooth inteiro com o usbipd é possível no kernel atual, mas é experimental, tira o Bluetooth do
  Windows e não serve para controles BLE (o kernel do WSL vem sem `UHID`). **[F]+[I]**

---

## 1. Como um controle Bluetooth genérico é exposto

```
CONTROLE ──(rádio)──► ADAPTADOR (USB, PCIe/UART interno) ──► pilha Bluetooth do SO
   │                                                             │
   │  Clássico (BR/EDR): perfil HID (HIDP)                       ├─► driver HID do transporte
   │  BLE: perfil HID over GATT (HOGP)                           │      ▼
   │                                                             │   núcleo HID do SO (descritor → campos)
   └─ USB (cabo): classe HID                                     │      ▼
                                                                 └─► APIs: HID bruto, entrada, jogos
```

1. O controle é **pareado** pelo sistema. O adaptador costuma ser um dispositivo USB interno, mesmo
   num notebook (por exemplo, o Bluetooth das placas Intel AX). **[B]**
2. A pilha Bluetooth do SO descobre que o controle oferece o serviço HID e carrega o driver HID
   daquele transporte. **[F]**
3. O núcleo HID lê o **descritor de relatório** (que diz "estes 16 bits são o eixo X, este bit é o
   botão 3") e entrega os dados como um dispositivo HID comum, igual a um conectado por USB. **[B]**
4. Por cima disso, o SO oferece APIs de níveis diferentes: relatórios brutos, eventos de entrada e
   APIs de jogos com mapeamento de "gamepad". **[F]**

Exceções que importam:

- **Controles Xbox.** Pelo cabo USB, não são HID: usam o protocolo próprio da Microsoft (driver
  XInput). Pelo Bluetooth, são HID, e o Windows os apresenta como "Bluetooth LE XINPUT compatible
  device", pelo driver `xinputhid`. **[F]** Os modelos atuais usam **BLE**. **[I]**
- **Protocolos do fabricante dentro do HID.** DualShock 4, DualSense e Switch Pro funcionam como
  HID genérico, mas com funções limitadas. Para giroscópio, vibração, luz e o modo completo, é
  preciso mandar comandos próprios, e é isso que os drivers HIDAPI do SDL fazem. **[F]**

## 2. Protocolos e camadas

| Camada | Bluetooth Clássico (BR/EDR) | Bluetooth LE | USB |
|--------|------------------------------|--------------|-----|
| Física/enlace | rádio BR/EDR, HCI até o adaptador | rádio LE, HCI | USB |
| Transporte | **L2CAP**, canais **PSM 0x11** (controle) e **PSM 0x13** (interrupção) **[F]** | **ATT/GATT** sobre L2CAP **[B]** | endpoints *interrupt* **[B]** |
| Perfil | **HIDP** (HID Profile) **[F]** | **HOGP**: serviço GATT **0x1812**, característica *Report Map* = descritor, *Report* com notificação **[F]** | classe HID (0x03) **[B]** |
| Descoberta | registro **SDP** com o descritor | *Report Map* lido pelo GATT | descritor USB |
| Dados | relatórios HID | relatórios HID | relatórios HID |

O ponto central: **as três colunas entregam os mesmos relatórios HID**. O que o programa precisa
saber é ler um relatório HID, ou usar uma API que já o leu.

- **Latência:**
  - BLE: depende do intervalo de conexão, mínimo de 7,5 ms pela especificação. **[B]**
  - BR/EDR: controles costumam reportar entre 4 e 8 ms. **[B]**
  - USB: 1 a 8 ms. **[B]**
  - Ordem de grandeza: todos são menores que um quadro a 60 Hz (16,7 ms). **[I]**

## 3. Como cada sistema expõe esses dispositivos

### Windows **[F]**

```
Bluetooth: BthUSB/BthPort ─┬─ HidBth.sys     (HID do Clássico, desde o Vista)
                           └─ BthLEEnum (ATT/GATT) ── HidBthLE.dll (HID sobre BLE, desde o Windows 8)
USB:       usbhid (HID)  │  xusb22 (Xbox por cabo, não é HID)
                 ▼
            HidClass.sys ── coleções HID ──► hid.dll (HidD_*, HidP_*), Raw Input, DirectInput
                 │                               xinputhid ──► XInput (Xbox por Bluetooth)
                 └──────────────────────────► Windows.Gaming.Input, GameInput
```

- O **acesso direto** ao serviço HID de um dispositivo BLE é negado aos aplicativos: as
  características dele voltam `DeniedBySystem`. Do Clássico, o Windows ocupa os PSMs do HID.
- **Windows.Gaming.Input** (inclusive `RawGameController`, que aceita controles genéricos) só
  entrega dados ao aplicativo **em foco**.
- **GameInput** é a API mais nova da Microsoft:
  - aceita entrada em segundo plano por opção (`GameInputFocusPolicy`);
  - reconhece HID genérico por um mapeamento no registro;
  - vem por um instalador separado e funciona desde o Windows 10 19H1.

### Linux **[F]**

```
btusb (adaptador) ─► BlueZ (bluetoothd, espaço de usuário)
   Clássico: UserspaceHID=true (padrão) → /dev/uhid → núcleo HID
             UserspaceHID=false         → hidp (módulo do kernel) → núcleo HID
   BLE:      HOGP no bluetoothd → /dev/uhid → núcleo HID
USB:        usbhid → núcleo HID │ xpad (Xbox por cabo)
núcleo HID → hid-generic / hid-playstation / hid-nintendo / ... → núcleo de entrada
          ├─► /dev/input/eventN  (evdev: eventos já interpretados)
          ├─► /dev/hidrawN       (hidraw: relatórios brutos)
          └─► /dev/input/jsN     (joydev, interface antiga)
```

- No BlueZ atual, **`UserspaceHID` vem como `true` por padrão**: até o HID do Clássico passa pelo
  `uhid`, em espaço de usuário. O BLE (HOGP) sempre passa por lá.
- É o único dos três sistemas em que o **acesso direto** é possível: um programa pode abrir
  sockets L2CAP nos PSMs 0x11 e 0x13, desde que o plugin de entrada do BlueZ não os ocupe.
- **[B]** Por padrão, só o root lê o `/dev/hidrawN`. Uma regra do udev (com `uaccess`) libera o
  acesso para o usuário logado; o pacote do Steam traz essas regras. Sem esse acesso, o SDL usa o
  evdev.

### macOS **[F]**

```
Bluetooth (Clássico e BLE) / USB ─► IOHIDFamily ─► IOHIDManager / IOHIDDevice (IOKit)
                                             └──► GameController.framework (GCController)
```

- O **GameController.framework** só lista controles **conhecidos** (Xbox, PlayStation, MFi e
  outros que a Apple reconhece). Um HID genérico pode parear e nunca aparecer como `GCController`.
- O caminho para controle genérico é o **IOKit HID** (`IOHIDManager`).
- Abrir dispositivos HID pode exigir a permissão de **Input Monitoring** (TCC). O SDL só toca em
  dispositivos com uso de joystick ou gamepad, para não disparar o pedido de permissão de teclado.
- O CoreBluetooth esconde o serviço HID (0x1812): **não há acesso direto**.

## 4. APIs para ler os dados

| Sistema | API | Nível | Controles genéricos | Em segundo plano | Observação |
|---------|-----|-------|---------------------|------------------|------------|
| Windows | `hid.dll` (HidD/HidP) + `ReadFile` | HID bruto | sim | sim | você interpreta o descritor (HidP ajuda) **[B]** |
| Windows | Raw Input | HID bruto (relatórios) | sim | com `RIDEV_INPUTSINK` | precisa de janela e laço de mensagens **[B]** |
| Windows | DirectInput 8 | eventos/estado | sim | sim | legado, mas ainda o que mais vê dispositivos genéricos **[B]** |
| Windows | XInput | gamepad | **só Xbox/XInput**, até 4 | sim | **[B]** |
| Windows | Windows.Gaming.Input | gamepad / bruto | `RawGameController` sim | **não** (precisa de foco) | **[F]** |
| Windows | GameInput | gamepad / bruto | sim (mapeamento) | por opção | **[F]** |
| Linux | evdev (`libevdev`) | eventos | sim | sim | já interpretado pelo driver do kernel **[B]** |
| Linux | hidraw | HID bruto | sim | sim | permissão do udev **[B]** |
| Linux | sockets L2CAP do BlueZ | **Bluetooth direto** | sim (Clássico) | sim | conflita com o plugin de entrada **[F]** |
| macOS | IOKit HID (`IOHIDManager`) | HID bruto e valores | sim | sim | permissão de Input Monitoring **[F]** |
| macOS | GameController.framework | gamepad | **só conhecidos** | configurável | **[F]** |

## 5. Abstrações multiplataforma

| Biblioteca | Em qual camada fica | Bluetooth? | O que entrega | Comentário |
|------------|--------------------|------------|---------------|------------|
| **SDL2 / SDL3** | por cima das APIs do SO **e** do HID bruto | sim, pelo SO | joystick bruto + **gamepad padronizado** (botão A, gatilho, analógico) com banco de mapeamentos | **[F]** No Windows usa HIDAPI, Raw Input, XInput, DirectInput, WGI e WinMM; no Linux, evdev e HIDAPI (hidraw); no macOS, IOKit, GameController e HIDAPI. Tem drivers próprios (PS4, PS5, Switch, Xbox, 8BitDo...) e **joysticks virtuais**. |
| GLFW | APIs do SO | sim, pelo SO | joystick + gamepad pelos mapeamentos do SDL | **[B]** Mais simples, sem drivers HIDAPI, sem virtuais, sem vibração. |
| **hidapi** | HID bruto | sim, pelo SO | relatórios HID | **[B]** Windows: `hid.dll`; Linux: hidraw ou libusb; macOS: IOHIDManager. Você interpreta cada controle. |
| libusb | USB bruto | **não** | transferências USB | **[B]** Não alcança dispositivo Bluetooth: o adaptador é USB, o controle não. |
| evdev / libevdev | eventos do kernel | sim | eventos | só Linux **[B]** |
| Raw Input / IOKit / GameInput | API do SO | sim | varia | um sistema cada |

## 6. Três níveis de acesso, separados

1. **Acesso direto ao Bluetooth.** O programa fala L2CAP (Clássico) ou GATT (BLE) com o controle e
   interpreta o HID sozinho.
   - **Linux:** possível, com sockets L2CAP e o plugin de entrada do BlueZ desligado.
   - **Windows e Apple:** bloqueado para o serviço HID.
   - É o nível de maior controle e de menor portabilidade, e quase nunca é necessário: o SO já faz
     o transporte melhor. **[F]+[O]**
2. **Acesso ao HID já tratado pelo SO** (hidapi, hidraw, `hid.dll`, IOKit). O SO cuidou do
   pareamento e do transporte; o programa recebe relatórios HID **iguais para USB e Bluetooth**.
   Controle total sobre o protocolo do controle (giroscópio, LEDs, modos), sem escrever pilha
   Bluetooth. Exige interpretar o descritor ou conhecer o protocolo de cada modelo. **[B]**
3. **Acesso aos eventos que o SO produziu** (evdev, XInput, DirectInput, WGI, GameInput,
   GameController.framework). O driver do SO já transformou o relatório em "botão 3 apertado", "eixo
   X = 0,4". É o mais simples, mas cada sistema numera os botões de um jeito, e é aí que entra o
   mapeamento de gamepad do SDL ou do GLFW. **[B]**

O SDL junta os níveis 2 e 3: usa o 2 (HIDAPI) para os controles que conhece e o 3 para o resto, e
entrega tudo como um gamepad padronizado. **[F]**

## 7. WSL2 em detalhe

**O que o WSL2 é, para os controles:** uma VM leve com kernel Linux próprio, **sem acesso aos
dispositivos do Windows**. Um controle pareado no Windows **não aparece** no WSL: não há
`/dev/input/event*` nem `/dev/hidraw*` para ele. **[B]**

### O WSL enxerga o controle Bluetooth pareado no Windows?

**Não.** Não existe repasse de HID do Windows para o WSL. O usbipd repassa **dispositivos USB**, e o
controle Bluetooth não é um dispositivo USB: só o adaptador é. **[B]+[I]**

### E o HID, sem levar o Bluetooth?

Não existe repasse de "dispositivo HID" do Windows para o WSL. **[B]** O equivalente prático é
**encaminhar o estado do controle pela rede** (seção 9C): o Windows lê, o Linux recebe.

### Repassar o adaptador Bluetooth inteiro com o usbipd-win

É **possível em tese, e mais viável hoje do que a maioria dos guias diz**:

- **[F]** O kernel atual do WSL (6.6.107.1, `config-wsl`) traz o Bluetooth como módulos:
  `CONFIG_BT=m`, `CONFIG_BT_BREDR=y`, `CONFIG_BT_LE=y`, `CONFIG_BT_HIDP=m`,
  `CONFIG_BT_HCIBTUSB=m`, `CONFIG_BT_INTEL=m`, `CONFIG_BT_RTL=m`, `CONFIG_BT_BCM=m`, além de
  `CONFIG_USBIP_VHCI_HCD=m`, `CONFIG_HIDRAW=y`, `CONFIG_HID_GENERIC=m`, `CONFIG_INPUT_EVDEV=m` e
  `CONFIG_INPUT_UINPUT=m`.
- **[F]** Desde o WSL 2.3.11 (kernel 6.6.36.3), o WSL vem com centenas de módulos carregáveis. Os
  guias que mandam recompilar o kernel para ter Bluetooth são anteriores a isso.

**As limitações são sérias:**

1. **[F]** **`CONFIG_UHID` não está habilitado.** Como o BlueZ usa `UserspaceHID=true` por padrão,
   o controle **não funciona sem mudar a configuração**: é preciso `UserspaceHID=false` em
   `/etc/bluetooth/input.conf`, para usar o `hidp` do kernel.
2. **[I]** **Controles BLE não funcionam**, porque o HOGP do BlueZ depende do `uhid`. Isso inclui os
   controles Xbox atuais via Bluetooth. Só os de Bluetooth Clássico funcionariam (DualShock 4,
   DualSense, Switch Pro e a maioria dos genéricos). Para BLE, só com um kernel próprio que tenha
   `CONFIG_UHID`.
3. **[B]** **O Windows perde o adaptador inteiro** enquanto ele estiver no WSL: fones, teclado e
   mouse Bluetooth param.
4. **[H]** O adaptador precisa do **firmware** dele (Intel, Realtek, Broadcom) disponível para o
   kernel do WSL. Não verifiquei de onde o kernel do WSL carrega firmware.
5. **[B]** O BlueZ precisa do `bluetoothd` e do D-Bus rodando no WSL, com systemd ou à mão. O
   controle precisa ser **pareado de novo** no Linux, porque as chaves do pareamento ficam no
   Windows.
6. **[I]** Tudo isso é refeito a cada `wsl --shutdown`: os anexos do usbipd não são persistentes.

**Veredito [O]:** só vale a pena se o programa **tiver** que falar Bluetooth no Linux (por exemplo,
para testar a própria pilha). Para receber os dados de um controle, é complicado demais e
excludente demais.

### Alternativas

| Caminho | Funciona com Bluetooth? | Custo |
|---------|-------------------------|-------|
| Controle **por cabo** + usbipd (o que o Tank 1990 já faz) | não (só USB) | baixo; o Windows perde o controle enquanto o jogo roda |
| Adaptador Bluetooth via usbipd | só Clássico, com `UserspaceHID=false` | alto; o Windows perde o Bluetooth |
| **Ponte: ler no Windows, encaminhar ao WSL** | **sim, qualquer um que o Windows enxergue** | um programa no Windows; ~1 ms a mais |
| Rodar o programa nativo no Windows | sim | nenhum, se o executável puder rodar (ver o Smart App Control abaixo) |

**[F] Smart App Control.** O Windows 11 com o Smart App Control ligado bloqueia executáveis sem
assinatura e sem reputação, como um `.exe` compilado em casa. **É por isso que o Tank 1990 roda no
WSL no Windows.** A mesma regra vale para o executável da ponte. Com o Smart App Control ligado,
sobram o cabo USB e o caminho experimental do adaptador.

**Por que a ponte não usa Windows.Gaming.Input nem PowerShell [F]+[I]:** o WGI só entrega dados ao
aplicativo em foco, e o foco é da janela do jogo, não da ponte. Um script PowerShell precisaria
chamar APIs nativas (`Add-Type`), o que compila e carrega uma DLL sem assinatura, também sujeita
ao Smart App Control. Com a mesma restrição, um executável SDL é mais simples e cobre mais controles.

### Rede entre Windows e WSL **[F]**

No modo de rede padrão (NAT), uma porta TCP aberta dentro do WSL fica acessível no Windows em
`127.0.0.1` (`localhostForwarding`, ligado por padrão). No modo espelhado (Windows 11 22H2 ou mais
novo), os dois lados se alcançam por `localhost`. Por isso, na ponte, o jogo no WSL **escuta** e o
programa no Windows **conecta**: funciona nos dois modos, sem regra de firewall.

## 8. Comparação

| Abordagem | Windows | Linux | macOS | WSL2 | Acesso direto ao BT | Via HID | Implementação | Portabilidade | Dependências | Latência | Limitações |
|-----------|---------|-------|-------|------|---------------------|---------|---------------|---------------|--------------|----------|------------|
| **SDL (gamepad)** | sim | sim | sim | só o que chega ao WSL (USB repassado ou virtual) | não | sim (HIDAPI + APIs do SO) | **fácil** | **alta** | SDL | a do SO (~1–8 ms) **[I]** | controles exóticos sem mapeamento viram joystick bruto |
| GLFW | sim | sim | sim | idem | não | parcial | fácil | alta | GLFW | idem | sem drivers HIDAPI, sem virtuais nem vibração |
| hidapi | sim | sim | sim | só USB repassado | não | **sim, bruto** | difícil (protocolo de cada controle) | alta | hidapi | mínima | permissões (udev, Input Monitoring); você interpreta tudo |
| evdev | — | sim | — | sim, com dispositivos repassados | não | não (eventos) | média | só Linux | libevdev | mínima | numeração de botões varia por driver |
| Raw Input / DirectInput / XInput | sim | — | — | — | não | Raw Input sim | média | só Windows | Win32 | mínima | XInput só Xbox; DirectInput legado |
| Windows.Gaming.Input | sim | — | — | — | não | `RawGameController` | média | só Windows | WinRT | mínima | **precisa de foco** |
| GameInput | sim (Win10 19H1+) | — | — | — | não | sim | média | só Windows | instalador próprio | mínima | genérico depende de mapeamento |
| IOKit HID / GameController | — | — | sim | — | não | IOKit sim | média | só macOS | frameworks do sistema | mínima | GC só conhecidos; Input Monitoring |
| Sockets L2CAP (BlueZ) | — | sim | — | com adaptador repassado | **sim** | você interpreta | **muito difícil** | **nenhuma** | BlueZ | mínima | só Clássico; conflita com o BlueZ |
| usbipd: controle por cabo | — | — | — | sim | não | sim (no Linux) | fácil | só WSL | usbipd-win | +USB/IP **[I]** | **não é Bluetooth** |
| usbipd: adaptador Bluetooth | — | — | — | experimental | sim (no Linux) | sim | difícil | só WSL | usbipd, BlueZ | +USB/IP **[I]** | sem BLE (`UHID`), Windows sem BT, firmware **[H]** |
| **Ponte Windows → WSL** | lê | — | — | **sim** | não | sim (SDL no Windows) | média | WSL (e rede em geral) | SDL nos dois lados | +~1 ms (TCP local) **[I]** | Smart App Control; um processo a mais |

## 9. A arquitetura mais adequada

**[O]** Para uma aplicação própria e multiplataforma: **SDL como camada de entrada, com o
transporte escondido abaixo dele**. Os motivos:

1. O transporte (USB, Clássico, BLE) é resolvido melhor pelo SO do que por qualquer código próprio,
   e em dois dos três sistemas nem é acessível. Descer ao Bluetooth só acrescenta trabalho e reduz a
   compatibilidade.
2. O SDL é a única biblioteca que junta os três níveis do item 6, com mapeamento padronizado e
   drivers para os controles populares, nas três plataformas.
3. O **joystick virtual** do SDL (2.24 ou mais novo) permite que uma fonte externa (rede, ponte,
   gravação, teste) apareça para o programa **exatamente como um controle físico**.

### A) A solução mais simples e portátil

**SDL GameController** (SDL2) ou **SDL Gamepad** (SDL3) direto, compilado nativo em cada sistema. O
SO pareia e trata o Bluetooth; o SDL entrega um gamepad padronizado. Funciona igual com cabo, com
Bluetooth Clássico e com BLE.

### B) A solução com maior controle sobre o hardware

**HID bruto pelo hidapi** (ou o HIDAPI do próprio SDL). Você recebe os relatórios do controle,
igual pelo cabo e pelo Bluetooth, e pode mandar comandos: modo completo do DualSense, giroscópio,
LEDs, vibração detalhada. É o nível mais baixo **que ainda é portátil**.

Abaixo disso, o Bluetooth direto (sockets L2CAP) **só existe no Linux**. **[F]**

### C) A melhor solução para o WSL2

**Ponte de entrada:** um programa no Windows lê os controles com o SDL (pela pilha Bluetooth do
Windows, que funciona) e envia o estado por TCP ao WSL. Lá, o programa cria um **joystick virtual
do SDL** para cada controle remoto. O código do programa no WSL não muda: ele vê um gamepad comum.

- **Se o Smart App Control impedir o executável da ponte:** controle por cabo com o usbipd.
- **Se o programa precisar mesmo de Bluetooth no Linux:** adaptador inteiro via usbipd, com as
  ressalvas da seção 7.

### D) Arquitetura de referência em camadas

```
┌──────────────────────────────────────────────────────────────────────┐
│ 6. Aplicação (regras do jogo)                                        │
│    pergunta: "o jogador 2 apertou tiro?"                             │
├──────────────────────────────────────────────────────────────────────┤
│ 5. Distribuição de dispositivos (Controllers)                        │
│    vaga ↔ jogador; teclado como reserva; hotplug                     │
├──────────────────────────────────────────────────────────────────────┤
│ 4. Gamepad padronizado (SDL_GameController)                          │
│    botões A/B/X/Y, gatilhos 0..32767, analógicos -32768..32767       │
├────────────────────────────┬─────────────────────────────────────────┤
│ 3a. Fontes do SDL          │ 3b. Fontes próprias → joystick virtual  │
│ HIDAPI, Raw Input, XInput, │ ponte de rede (NetPad), gravação, teste │
│ DirectInput, WGI, evdev,   │ (SDL_JoystickAttachVirtualEx)           │
│ IOKit, GameController      │                                         │
├────────────────────────────┴─────────────────────────────────────────┤
│ 2. SO: núcleo HID, drivers (HidClass / hid-generic / IOHIDFamily)    │
├──────────────────────────────────────────────────────────────────────┤
│ 1. Transporte: USB │ Bluetooth Clássico (HIDP) │ BLE (HOGP)          │
└──────────────────────────────────────────────────────────────────────┘
```

**Regras da arquitetura:**

- As camadas 5 e 6 só conhecem a camada 4. **Nenhuma decisão de jogo depende do transporte.**
- O transporte é **informação de diagnóstico** (USB, Bluetooth, virtual), tirada do GUID do SDL:
  os primeiros 16 bits são o barramento (`0x03` USB, `0x05` Bluetooth, `0xFF` virtual). **[F]**
- Uma fonte nova entra na camada 3b como joystick virtual: o resto não muda.

## 10. Como o Tank 1990 implementa isso

| Peça | Arquivo | Camada |
|------|---------|--------|
| Distribuição jogador ↔ dispositivo | `src/controllers.*` (já existia) | 5 |
| Informação do dispositivo (barramento, VID:PID) | `src/input/pad_info.*` | diagnóstico |
| Protocolo da ponte | `src/input/pad_protocol.h` | 3b |
| Socket TCP portátil (Winsock e BSD) | `src/input/pad_socket.*` | 3b |
| Receptor no jogo: controles remotos → joysticks virtuais | `src/input/netpad.*` | 3b |
| Ponte: lê os controles com o SDL e envia | `tools/padbridge.cpp` | 3a → rede |
| Diagnóstico: lista os controles, o barramento e os eventos | `tools/padprobe.cpp` | 4 |

- **Linux e macOS:** o jogo usa o SDL direto. Controle por cabo ou Bluetooth, pareado no sistema,
  funciona sem nada a mais.
- **Windows nativo (MSYS2/MinGW):** idem.
- **Windows com WSL** (o caminho do `install.cmd`):
  - o `install.cmd` também gera o `padbridge.exe` (compilado no próprio WSL, com o MinGW);
  - o `play.cmd` abre a ponte em segundo plano antes do jogo;
  - o jogo, no WSL, escuta a porta e cria os controles virtuais;
  - os controles por cabo continuam indo pelo usbipd, e os por Bluetooth vão pela ponte.

Os detalhes de uso estão no README (seção "Controles por Bluetooth").

### O que foi verificado, e o que não

| Verificação | Resultado |
|-------------|-----------|
| Ponte → `NetPad` → controle virtual → eventos de gamepad, no Linux (`make pad-selftest`) | **passou** |
| O controle remoto chega ao `Controllers` do jogo como "PAD 1", com o teclado de reserva, e sai quando a ponte sai (teste com ASan) | **passou** |
| `padbridge.exe` (compilado com o MinGW, Winsock) rodando no Wine → `padprobe` nativo do Linux | **passou** |
| `padprobe.exe` (receptor no Windows: Winsock sem bloquear e controle virtual) no Wine ← ponte do Linux | **passou** |
| Jogo inteiro compilado como `.exe` nativo do Windows (MinGW, SDL2 2.32.10), sem warnings | **passou** |
| CI: Linux, macOS, Windows (MSYS2) e `wsl-setup.sh` num Alpine, com o `pad-selftest` | roda em cada PR |
| Controle Bluetooth **de verdade** em cada sistema | **não verificado** (sem hardware nesta investigação) |
| `localhostForwarding` do WSL com o jogo escutando em `0.0.0.0` | documentado pela Microsoft e pelo wsl.dev; **não testado num WSL real** |
| Comportamento do Smart App Control com o `padbridge.exe` | inferido (é um executável sem assinatura); **não testado** |
| Adaptador Bluetooth repassado ao WSL pelo usbipd | **hipótese** (seção 7): não implementado, não testado |

O Wine não é o Windows: ele prova o código do Winsock e do SDL, não os drivers Bluetooth do
Windows. É o Windows da CI que fecha essa lacuna, ainda sem controle físico.

### Exemplos mínimos

**Camada 4: ler um gamepad, qualquer transporte (SDL2)**

```cpp
SDL_Init(SDL_INIT_GAMECONTROLLER);
SDL_GameController* pad = nullptr;
for(SDL_Event e; ; ) {
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_CONTROLLERDEVICEADDED && !pad) pad = SDL_GameControllerOpen(e.cdevice.which);
        if(e.type == SDL_CONTROLLERBUTTONDOWN) printf("botão %d\n", e.cbutton.button);
    }
    if(pad) printf("LX=%d\n", SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX));
}
```

**Diagnóstico: transporte a partir do GUID (SDL2)**

```cpp
SDL_JoystickGUID g = SDL_JoystickGetDeviceGUID(index);
Uint16 bus = g.data[0] | (g.data[1] << 8);       // 0x03 USB, 0x05 Bluetooth, 0xFF virtual
// No SDL3: SDL_GetJoystickConnectionState(joystick) → WIRED / WIRELESS
```

**Camada 3b: uma fonte externa vira um gamepad comum (SDL ≥ 2.24)**

```cpp
SDL_VirtualJoystickDesc d;
SDL_zero(d);
d.version  = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
d.type     = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
d.naxes    = SDL_CONTROLLER_AXIS_MAX;      // máscaras zeradas: índice i = eixo/botão i
d.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
d.name     = "Remoto: Wireless Controller";
int index = SDL_JoystickAttachVirtualEx(&d);
SDL_Joystick* j = SDL_JoystickOpen(index);
SDL_JoystickSetVirtualButton(j, SDL_CONTROLLER_BUTTON_A, 1);   // aparece como botão A
SDL_JoystickSetVirtualAxis(j, SDL_CONTROLLER_AXIS_LEFTX, 16000);
```

**Camada 2: relatório HID bruto (hidapi), igual por cabo ou Bluetooth**

```cpp
hid_device* h = hid_open(0x054c, 0x09cc, nullptr);   // VID:PID (ex.: DualShock 4)
unsigned char buf[78];
int n = hid_read_timeout(h, buf, sizeof buf, 100);  // o formato depende do controle e do modo
```

## Fontes

- Configuração do kernel do WSL 6.6 (`config-wsl`): https://raw.githubusercontent.com/microsoft/WSL2-Linux-Kernel/linux-msft-wsl-6.6.y/arch/x86/configs/config-wsl
- Repositório do kernel do WSL (módulos, `modules.vhdx`): https://github.com/microsoft/WSL2-Linux-Kernel
- Módulos carregáveis no WSL 2.3.11: https://www.phoronix.com/news/Microsoft-WSL-2.3.11
- BlueZ, `profiles/input/input.conf` (`UserspaceHID`, `ClassicBondedOnly`): https://raw.githubusercontent.com/bluez/bluez/master/profiles/input/input.conf
- BlueZ, PSMs do HID (`profiles/input/device.h`): https://kernel.googlesource.com/pub/scm/bluetooth/bluez/+/master/profiles/input/device.h
- usbipd-win (README, anexo ao WSL): https://github.com/dorssel/usbipd-win
- Bluetooth via usbipd no WSL (Espressif, ainda pedindo kernel próprio): https://docs.espressif.com/projects/esp-matter/en/latest/esp32/using_chip_tool.html
- Pilha Bluetooth do Windows (HidBth, BthLEEnum): https://learn.microsoft.com/windows-hardware/drivers/bluetooth/bluetooth-driver-stack
- Transportes HID do Windows (HidBth, HidBthLE): https://learn.microsoft.com/en-sg/WINDOWS-HARDWARE/DRIVERS/hid/hid-transports
- `RawGameController` e o foco: https://learn.microsoft.com/en-us/uwp/api/windows.gaming.input.rawgamecontroller
- GameInput, política de foco: https://learn.microsoft.com/en-in/gaming/gdk/docs/reference/input/gameinput-v2/enums/gameinputfocuspolicy-v2
- GameInput no PC (HID genérico, Windows 10 19H1, instalador): https://developer.microsoft.com/en-us/games/articles/2025/03/gdc-2025-gameinput-pc-expands-to-meet-your-needs/
- Acesso negado ao serviço HID sobre GATT no Windows: https://devzone.nordicsemi.com/f/nordic-q-a/41530/characteristics-in-hid-service-access-denied-on-windows-app/161743
- HID (0x1812) escondido no CoreBluetooth: https://developer.apple.com/forums/thread/725238
- GameController.framework só lista controles conhecidos: https://developer.apple.com/forums/thread/763679
- SDL e o Input Monitoring no macOS: https://discourse.libsdl.org/t/sdl-dont-enumerate-hid-devices-on-macos-if-we-dont-have-input-monitoring-permissions/38822
- Controle Xbox via Bluetooth no Windows (`xinputhid`): https://treexy.com/products/driver-fusion/database/human-interface-devices/microsoft/xbox-wireless-controller/
- Smart App Control: https://learn.microsoft.com/en-gb/windows/security/book/application-security-application-and-driver-control
- Rede do WSL (`localhostForwarding`, modo espelhado): https://wsl.dev/technical-documentation/localhost/
- Código-fonte do SDL2 (joystick virtual, barramento no GUID, drivers): https://github.com/libsdl-org/SDL/tree/SDL2/src/joystick
- SDL3, `SDL_GetJoystickConnectionState`: https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_joystick.h
- Versões do SDL (tags `release-2.32.10` e `release-3.4.18`): https://github.com/libsdl-org/SDL/tags
