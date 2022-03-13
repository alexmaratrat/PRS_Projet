# Etat du projet

Programme :
- Taille fichier >>
- Timer

k : Numéro du premier segment non acquitté en contigu (la valeur du ACK ou non ACK dans le tableau ack est à l'indice **0**)

cwnd : taille de la fenêtre (=5)

Init
[-4 -4 -4 -4 -4]

k=1
Transmission
[-1 -1 -1 -1 -1]
segments 1,2,3,4 et 5

Réception ACKs
Je reçois ACK1
[0 -1 -1 -1 -1]
Je reçois ACK3
[0 0 0 -1 -1]
Je reçois ACK3 (x3)
[0 0 0 -4 -1]

Management cwnd
[-4 -1 -4 -4 -4]

k =4 , déclage de 3 (k-1)

Transmission
[-1 -1 -1 -1 -1]
segments 4,5,6,7 et 8

Réception ACKs
Je reçois ACK7
[0 0 0 0 -1]
Je reçois ACK7x4
[0 0 0 0 -4]

Management cwnd
[-4 -4 -4 -4 -4]
k=8


Questions à poser
> Faut il réestimer le RTT au cours du programme ? Si oui, à quelle fréquence? Pas besoin
> Demander à Max s'il peut check comment faire pour se connecter aux pc du départ en vpn pour lancer les tests

Piste d'amélioration

- Faire du multithreading pour améliorer la gestion de la fenêtre --> un thread pour s'occuper uniquement de la réception des acks et du traitement (extraction numéro segment) pour optimiser
- Pouvoir ajuster la fenêtre client
( tileout)


Note à soi même en C pour plus tard
- Pourquoi strncpy(string, "X",1) marche et strncpy(string, 'X',1) non ??? '' = un char et pas considéré comme un string? Bref ça m'a rendu fou.
Réglé --> On attend un *char et en écrivant "" on initialise directement une chaîne de caractère, dont le point d'entrée est un pointeur vers la chaîne, alors qu'en mettant '' --> on donne direct un char, et non un pointeur vers une chaîne de caractère. Pas d'erreur lors de la compilation, on voit
- memset('\0') ou memset(0) revient en même car dans table ASCII, code ASCII pour int = 0 équivalent à chaîne '\0' (déterminant la fin du string)

- Utilisation de memcpy plutôt strncpy ou autre --> déplacement direct case mémoire = pas de couilles. Hypothèse = fctions pour les strings manipulent les chaînes de caractères et certains chunks sont tronqués ou modifié? lorsqu'on envoie les segments
