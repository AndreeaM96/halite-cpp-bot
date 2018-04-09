#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>
#include <algorithm>
#include <tuple>
#include <climits>

#include "hlt.hpp"
#include "networking.hpp"

/*O HEURISTICA este o functie sau formula care determina cea mai optima alegere/directie
dupa cateva criterii. */

struct Territory{
  int id;
  int territory;

  Territory(int id, int territory): id(id), territory(territory){}


};

struct Inamic{
  int id;
  int frontiera;
  int putere;
  hlt::Location center;

  Inamic(int id, int frontiera, int putere, hlt::Location &center): id(id),
                                                                    frontiera(frontiera),
                                                                    putere(putere),
                                                                    center(center){}

  bool operator < (Inamic &other) const {
    return frontiera < other.frontiera;
  }
};

struct Vecin{
  hlt::GameMap presentMap;
  hlt::Location locatie;
  unsigned char direction;
  hlt::Location center;


  Vecin(hlt::GameMap &presentMap, hlt::Location &locatie,
        unsigned short direction, hlt::Location &center): locatie(locatie),
                                                          direction(direction),
                                                          presentMap(presentMap),
                                                          center(center){}

  bool operator < (Vecin &other){
    /*Cel mai bun vecin ales este ales dupa o heuristica. Vezi heuristic_two pentru explicatie. */
    double first_production = presentMap.getSite(locatie).production;
    double second_production = presentMap.getSite(other.locatie).production;
    double first_strength = presentMap.getSite(locatie).strength;
    double second_strength = presentMap.getSite(other.locatie).strength;

    if(second_strength == 0) return false;

    return (first_production/first_strength >
            second_production/second_strength);
  }

};



int getTerritory(int id, std::vector<Territory> &ter){
  for(int i =0 ; i < ter.size(); i++){
    if(ter[i].id == id) return ter[i].territory;
  }
  return 0;
}

bool heuristic_one(Vecin &first, Vecin &second){
  /*Consider cea mai optima alegere, vecinul straini care are productia cea mai mare*/
  hlt::GameMap presentMap = first.presentMap;
  return (presentMap.getSite(first.locatie).production > presentMap.getSite(second.locatie).production);
}

bool heuristic_two(Vecin &first, Vecin &second){
  /*Consider cea mai optima alegere, vecinul straini care are productia cea mai mare si distanta
  de la celula interna cautata cea mai mica (se poate ajunge cel mai repede la ea).
  Astfel heuristica lui h(x) (cat de buna este alegerea adica) este direct proportionala cu
  productivitatea (vrem productivitate mare) si invers proportionala cu distanta de la nodul
  intern la celula, deoarece se poate ajunge mai repede la ea daca distanta este mai mica
  adica h(x) = productivitate(x) / distanta(centru, x) */
  hlt::GameMap presentMap = first.presentMap;
  double first_production = presentMap.getSite(first.locatie).production;
  double second_production = presentMap.getSite(second.locatie).production;
  double first_distance = presentMap.getDistance(first.locatie, first.center);
  double second_distance = presentMap.getDistance(second.locatie, second.center);

  return (first_production/first_distance > second_production/second_distance);

}

bool heuristic_three(Vecin &first, Vecin &second){
  /*Considerm cea mai buna alegere ca si in heuristic_two doar ca luam in calcul si puterea lui.
  Daca vecinul strain are o putere mai mica atunci poate fi batut mai repede deci euristica lui
  este indirect proportionala cu puterea lui. Exact ca in heuristic two doar ca impartim si la putere. */
  hlt::GameMap presentMap = first.presentMap;
  double first_production = presentMap.getSite(first.locatie).production;
  double second_production = presentMap.getSite(second.locatie).production;
  double first_distance = presentMap.getDistance(first.locatie, first.center);
  double second_distance = presentMap.getDistance(second.locatie, second.center);
  double first_strength = presentMap.getSite(first.locatie).strength;
  double second_strength = presentMap.getSite(second.locatie).strength;


  return (first_production/(first_distance * first_strength) > second_production/(second_distance * second_strength));


}

int reverse_direction(int i){
  switch (i) {
    case NORTH: return SOUTH;
    case SOUTH: return NORTH;
    case EAST: return WEST;
    case WEST: return EAST;
  }
}

unsigned char look_for_border(hlt::Location center, hlt::GameMap presentMap,
                                std::vector<Inamic> inamici, std::vector<Territory> ter, int myID){
  /* Functie folosita pentru a determina directia unei celule interne dupa o functie
  de comparare (o heuristica). Heuristica decide care este directia optima spre care
  trebuie sa se miste celula interna luand in considerare parametrii precum productivitate,
  distanta fata de celula interna center, puterea celulelor inamice straine. */
  unsigned short direction = 0;
  int max_distance = std::min(presentMap.width, presentMap.height)/5;
  //Caut intr-un sfert din distanta hartii in jurul celulei center. Parametru modifiabil. Testati cu diverse divizuni
  //ale hartii!
  int distance = 0;
  int min_direction;
  int min_strength = INT_MAX;
  std::vector<hlt::Location> direction_borders;
  //Va retine vecinii nord sud est vest pentru celula center la o distanta max_distance
  std::vector<Vecin> vecini_neutri;
  std::vector<Vecin> vecini_inamici;

  //Va retine vecinii care nu sunt ai mei (owner != myID) cei mai apropiati de celula center
  //In nord, sud, est, vest daca au o distanta fata de celula center mai mica decat max_distance

  for(int d = 0; d < 5; d++){
    direction_borders.push_back(center);
  }
  //Incarc vectorul cu celula center;

  for(int d = 1; d < 5; d++){
    //Pentru fiecare directie NORD SUD EST VEST
    while(distance < max_distance){
      //Cat timp distanta e mai mica decat distana maxima pe care caut
      direction_borders[d] = presentMap.getLocation(direction_borders[d], d);
      int found_id = presentMap.getSite(direction_borders[d]).owner;
      if(found_id == 0){
        //Daca s-a gasit ca celula nu este a mea, la distanta "distance", atunci
        //aduaga in vector si iesi din while
        vecini_neutri.push_back(Vecin(presentMap, direction_borders[d], d, center));
        break;
      }else if(found_id != myID){
        vecini_inamici.push_back(Vecin(presentMap, direction_borders[d], d, center));
        break;
      }
      distance++;
    }
    distance = 0;
  }

  if(vecini_neutri.size() != 0){
    //Daca s-au gasit in distanta maxima fata de centru vecinii care nu sunt celulele
    //Mele sorteaza vectorul de vecini straini dupa o heuristica
    //Vezi functiile de heuristica, am definit trei astfel de functii, puteti incerca cu
    //fiecare, inlocuind functia heuristic_three cu heuristic_two si heuristic_one
    std::sort(vecini_neutri.begin(), vecini_neutri.end(), heuristic_three);
    return vecini_neutri.begin()->direction;
  }else if(vecini_inamici.size() != 0){
    /* O secventa care vede in vectorul de inamici gasit din bfs-ul initial,
    locul inamicului si in ce front intra aceasta. */
    int i_max = -1;

    float heuristica_max = 0;
    for(int i = 0; i < vecini_inamici.size(); i++){
      int found_id = presentMap.getSite(vecini_inamici[i].locatie).owner;
      for(int j = 0; j < inamici.size(); j++){
        if(inamici[j].id == found_id){
          float heuristica_noua;
          heuristica_noua = (getTerritory(found_id, ter)*getTerritory(found_id, ter))
                            /(presentMap.getDistance(inamici[j].center, vecini_inamici[i].locatie)
                            * presentMap.getDistance(inamici[j].center, vecini_inamici[i].locatie) * inamici[j].putere);
          if(heuristica_noua > heuristica_max){
            heuristica_max = heuristica_noua;
            i_max = i;
          }
        }
      }
    }
    if(i_max > 0){
      return vecini_inamici[i_max].direction;
    }
  }
    //Daca nu am vecini straini, vezi care este celula cea mai slaba si cedeaza-i putere
    for(int d = 1; d < 5; d++){
      if(presentMap.getSite(direction_borders[d]).strength < min_strength){
        min_strength = presentMap.getSite(direction_borders[d]).strength;
        min_direction = d;
      }
    }
    return min_direction;
}

unsigned char look_for_enemy(hlt::Location center, hlt::GameMap presentMap, int myID){
  /*Verifica numarul de inamici din fata noastra si pune piesa astfel incat sa fie maximizata
  dauna. */
  hlt::Location celula_din_fata;
  int inamici = 0;
  int damage = 0;
  int best_dir = 5;
  int max_damage = 0;
  for(int i = 1; i < 5; i++){
    celula_din_fata = presentMap.getLocation(center, i);
    if(presentMap.getSite(celula_din_fata).owner == 0 &&
        presentMap.getSite(celula_din_fata).strength < presentMap.getSite(center).strength){
          for(int j = 1; j < 5; j++){
            if(presentMap.getSite(presentMap.getLocation(celula_din_fata, j)).owner!= myID &&
                presentMap.getSite(presentMap.getLocation(celula_din_fata, j)).owner != 0){
                  inamici++;
                }
          }
          damage = inamici * presentMap.getSite(center).strength;
          if(damage > max_damage){
            max_damage = damage;
            best_dir = i;
          }
          if(inamici == 3){
            return i;
          }
        }
  }
  return best_dir;
}



int main() {
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);
    sendInit("Graph d3stroy3r$");
    std::vector<Vecin> *vecini_straini;
    hlt::Location next;
    hlt::Location current_position;
    std::set<hlt::Move> moves;
    int mature_cell_strength;
    std::ofstream out("Log");
    std::vector<Inamic> inamici;
    float _omega;
    int search_distance = 10;
    int past_territory = 0;
    std::vector<Territory> territory;
    while(true) {
        moves.clear();
        getFrame(presentMap);
        /*Cu cat harta este mai mare cu atat preferam sa asteptam mai mult deoarece
        intram mai greu in contact cu inamicul si avem timp sa ne fortificam
        piesele centrale. */
        _omega = std::max(presentMap.height, presentMap.width)/10;

        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                current_position = {b, a};
                bool registed = false;
                for(int i = 0; i < territory.size(); i++){
                  if(territory[i].id == presentMap.getSite(current_position).owner){
                    territory[i].territory++;
                    registed = true;
                    break;
                  }
                }
                if(!registed){
                  territory.push_back(Territory(presentMap.getSite(current_position).owner, 1));
                }
            }
          }



        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {

                /* Secventa care produce vectorul de frontiere. Un Inamic are un centru
                si o putere asociata. Daca se gaseste o celula inamica, care este la
                o distanta mai mica decat 3 de centrul unei celule inamice atunci puterea
                celuei inamice va creste cu puterea celulei noi.
                Altfel se va adauga celula inamica. */

                if(presentMap.getSite({b, a}).owner == myID){
                  for(unsigned short i = 1; i < 5; i++){
                  hlt::Location current = {b, a};
                  next = presentMap.getLocation(current, i);
                  for(int j = 0; j < search_distance; j++){
                      next = presentMap.getLocation(next, i);
                  }
                   int found_id = presentMap.getSite(next).owner;
                    if(found_id != myID && found_id != 0){
                        bool registed = false;
                        for(int i = 0; i < inamici.size(); i++){
                          if(inamici[i].id == found_id &&
                              presentMap.getDistance(inamici[i].center, next) < 3){
                            inamici[i].frontiera++;
                            inamici[i].putere = inamici[i].putere + presentMap.getSite(next).strength;
                            registed = true;
                            break;
                          }
                        }
                        if(!registed){
                          inamici.push_back(Inamic(found_id, 1, 0, next));
                        }
                  }
                }
              }
            }
          }

        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                if (presentMap.getSite({ b, a }).owner == myID) {

                  mature_cell_strength = presentMap.getSite({b,a}).production;
                  if(presentMap.getSite({b, a}).strength > _omega*mature_cell_strength){
                  vecini_straini = new std::vector<Vecin>();
                  for(unsigned short i = 1; i < 5; i++){
                    /*Scanam vecinii din NORD SUD EST VEST, daca vecinii sunt straini aduagam
                    in vectorul vecini_straini altfel nu adaugam in acest vector.
                    Daca nu prezinta vecini_straini, toti vecinii celulei sunt ai mei si deci
                    celula este interna, altfel celula este externa. */
                    next = presentMap.getLocation({b, a}, i);
                    current_position = {b, a};
                    if(presentMap.getSite(next).owner != myID){
                      vecini_straini->push_back(Vecin(presentMap, next, i, current_position));
                    }
                    //
                  }
                  if(vecini_straini->size() == 0){
                    /*Daca nu exista vecini straini, celula e interna. Celula merge spre directia
                    determinata de look_for_border dupa o heuristica data. */
                    moves.insert({{b, a}, look_for_border({b, a}, presentMap, inamici, territory, myID) });
                  }else{
                    /*Daca ceula este externa mergi catre vecinul cel mai optim, vecinul
                    cel mai optim este determinat de operatorul "<" din clasa Vecin dupa o heuristica.*/
                    unsigned char enemy_dir = look_for_enemy({b, a}, presentMap, myID);
                    if(enemy_dir != 5){
                      moves.insert({{b, a}, enemy_dir});

                    }else{
                      std::sort(vecini_straini->begin(), vecini_straini->end());
                      if(presentMap.getSite(vecini_straini->begin()->locatie).strength <
                        presentMap.getSite({b, a}).strength){
                          moves.insert({{b, a}, vecini_straini->begin()->direction});
                      }else{
                        moves.insert({{b, a}, STILL});
                      }
                    }
                  }
                }
              }
              }
            }
      sendFrame(moves);
      inamici.clear();
      territory.clear();
    }

    return 0;
}
