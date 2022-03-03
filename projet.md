# Etat du projet

Programme :
- Taille fichier >>
- Timer


Questions à poser
> Faut il réestimer le RTT au cours du programme ? Si oui, à quelle fréquence? Pas besoin
> Demander à Max s'il peut check comment faire pour se connecter aux pc du départ en vpn pour lancer les tests

Piste d'amélioration

- Faire du multithreading pour améliorer la gestion de la fenêtre --> un thread pour s'occuper uniquement de la réception des acks et du traitement (extraction numéro segment) pour optimiser
- Pouvoir ajuster la fenetre client


Note à soi même en C pour plus tard
- Pourquoi strncpy(string, "X",1) marche et strncpy(string, 'X',1) non ??? '' = un char et pas considéré comme un string? Bref ça m'a rendu fou.
Réglé --> On attend un *char et en écrivant "" on initialise directement une chaîne de caractère, dont le point d'entrée est un pointeur vers la chaîne, alors qu'en mettant '' --> on donne direct un char, et non un pointeur vers une chaîne de caractère. Pas d'erreur lors de la compilation, on voit
- memset('\0') ou memset(0) revient en même car dans table ASCII, code ASCII pour int = 0 équivalent à chaîne '\0' (déterminant la fin du string)

- Utilisation de memcpy plutôt strncpy ou autre --> déplacement direct case mémoire = pas de couilles. Hypothèse = fctions pour les strings manipulent les chaînes de caractères et certains chunks sont tronqués ou modifié? lorsqu'on envoie les segments
