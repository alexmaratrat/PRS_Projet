# Etat du projet

Programme :
* Première estimation de RTT
* Socket de contrôle : reçoit premier ACK000000 mais rien après : done
* Erreur de segmentation lors de l'accès aux cases mémoire des segments


Questions à poser
> Faut il réestimer le RTT au cours du programme ? Si oui, à quelle fréquence? Pas besoin
> Demander à Max s'il peut check comment faire pour se connecter aux pc du départ en vpn pour lancer les tests

Piste d'amélioration

- Faire du multithreading pour améliorer la gestion de la fenêtre --> un thread pour s'occuper uniquement de la réception des acks et du traitement (extraction numéro segment)
- Pouvoir ajuster la fenetre client


Note à soi même en C pour plus tard
- Pourquoi strncpy(string, "X",1) marche et strncpy(string, 'X',1) non ??? '' = un char et pas considéré comme un string? Bref ça m'a rendu fou.
Réglé --> On attend un *char et en écrivant "" on initialise directement une chaîne de caractère, dont le point d'entrée est un pointeur vers la chaîne, alors qu'en mettant '' --> on donne direct un char, et non un pointeur vers une chaîne de caractère.
