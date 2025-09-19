Nous avons finalisé les pages suivantes :

Page d’accueil

Page de configuration des scénarios

Page administrateur, dont les sous-sections :

Configuration générale

Paramètres réseau

Nous travaillons actuellement sur la page des paramètres système.

Cette page permet notamment d’ajuster la luminosité de l’écran, et devait initialement intégrer un gestionnaire de fichiers pour :

Renommer

Supprimer

Ajouter des fichiers image, vidéo et GIF dans la mémoire du CPU

Cependant, ni l’interface web utilisée sur la Raspberry Pi Pico, ni l’application développée par l’informaticien ne prennent en charge cette fonctionnalité.

Nous avons donc décidé de l’intégrer directement dans l’interface web. Pour cela, nous avons commencé à explorer l’utilisation de FFmpeg, un utilitaire puissant permettant de manipuler les flux vidéo, les images, et bien plus.

Grâce à cette intégration, notre interface offrira une fonctionnalité native capable d’adapter automatiquement tout type de fichier multimédia (image, vidéo, GIF) aux dimensions et formats compatibles avec le CPU.
