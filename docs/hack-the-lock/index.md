# Pentest candado Bluetooth OKLOK

El objetivo de este Test de Penetración (Pentest) consiste en ver si somos capaces de encontrar la clave de desbloqueo de un candado inteligente para poder abrirlo mediante conexión bluetooth sin permiso del usuario.


<p align="center">
<img src="oklok.png"  width="60%" height="30%">
</p>

El candado objetivo, modelo OKLOK Padlock, es un candado que se puede desbloquear mediante huella digital y usando una [aplicación android](https://play.google.com/store/apps/details?id=com.oklok.lock&hl=en&gl=US) diseñada para tal efecto. 

Las versiones antiguas del candado tienen una vulnerabilidad conocida y documentada ([TFM Jesus Alberto Tejedor Doria, 2020](https://eprints.ucm.es/62476/1/JESUS_ALBERTO_TEJEDOR_DORIA_Entrega_Final_TFM_Pentesting_Device_IoT_Smart_Doorlock_4286353_1929042718.pdf)) que permitía averiguar la clave de desbloqueo de una manera relativamente sencilla. Sin embargo, el fabricante ha actualizado la aplicación complicando la obtención de dicha clave.

El objetivo de este taller consiste en ser capaces de encontrar la clave de acceso mediante el estudio e instrumentación de la nueva aplicación. Para ello os explicaré cómo se conseguía la clave usando la aplicación antigua, con la esperanza de que algún hacker sea capaz de obtener claves nuevas de los candados que tenemos en la FDI.