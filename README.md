# A propos de SLS

SRT Live Server (SLS) est un serveur open-source de _live streaming_ à faible latence basé sur le protocole SRT (Secure Reliable Transport).

Dans de bonnes conditions, la latence du transport par SLS est inférieure à 1 seconde sur Internet. Cependant, il faut ajouter à cela l'éventuel temps d'encodage et de décodage de la vidéo. Le temps de latence totale est alors de 2 à 3 secondes.

## Requirements

Pour compiler SLS, il faut installer SRT au préalable. Voir [SRT](https://github.com/Haivision/srt) pour les détails.

Par ailleurs, SLS ne peut être compilé que sur un système d'exploitation Unix ou Linux, tels que macOS, Debian, CentOS, etc.

## Compilation

Après avoir cloné et compilé SRT, il faut cloner SLS, puis compiler le code source avec la commande suivante :
```bash
sudo make
```

Un dossier `bin` est alors créé, contenant l'exécutable `sls`.

## Configuration

Pour la configuration de SLS, référez-vous au [Wiki](https://github.com/Le-JDL-La-Roche/SRT-Live-Server/wiki).

## Utilisation

```bash
cd bin
```

### Aide

```bash
./sls -h
```

### Démarrer le serveur

```bash
./sls -c ../sls.conf
```

## Tester

SLS supporte uniquement le format de streaming video MPEG-TS.

### Tester avec FFmpeg

Vous pouvez utiliser [FFmpeg](https://github.com/FFmpeg/FFmpeg) envoyer votre flux vidéo à SLS.

FFmpeg est disponible sur les distributions Linux basées sur Debian :

```bash
sudo apt install ffmpeg
```

Pour envoyer un flux vidéo à SLS, utilisez la commande suivante :

```bash
ffmpeg -re -f v4l2 -i "<input_video>" -vcodec libx264 -preset ultrafast -tune zerolatency -flags2 local_header -acodec libmp3lame -g 1 -f mpegts "srt://<sls_ip>:<port>?streamid=<domain_publisher>/<app_publisher>/<name>"
```

`<input_video>` est le chemin vers votre source vidéo.

`<sls_ip>` et `<port>` sont l'adresse IP et le port de votre serveur SLS.

`<domain_publisher>` et `<app_publisher>` sont les paramètres de l'URL de streaming, à configurer dans le fichier `sls.conf`.

`<name>` est le nom du flux vidéo.

Pour lire la vidéo avec FFplay :

```bash
ffplay -i "srt://<sls_ip>:<port>>?streamid=<domain_player>/<app_player>/<name>"
```

`<domain_player>` et `<app_player>` sont les paramètres de l'URL de streaming, à configurer dans le fichier `sls.conf`.

`<name>` est le nom du flux vidéo, et doit correspondre au nom indiqué lors de l'envoi du flux.

### Tester avec OBS Studio

OBS Studio (version >= 25.0) supporte l'envoie de flux vidéo par le biais du protocole SRT. Pour cela, configurez le service de streaming sur "Personnalisé..." (ou "Custom...") et entrez l'URL suivante :

```
srt://<sls_ip>:<port>?streamid=<domain_publisher>/<app_publisher>/<name>
```

Pour lire la vidéo avec VLC, cliquez sur "Media" > "Ouvrir un flux réseau..." et entrez l'URL suivante :

```
srt://<sls_ip>:<port>?streamid=<domain_player>/<app_player>/<name>
```