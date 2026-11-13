// ============================================================================
//  GESTION D'UNE AGENCE DE VOYAGE (Cotonou, Benin) - Application console C++
//  Compilation : g++ -std=c++17 -o agence agence_voyage.cpp
//  Execution   : ./agence        (donnees sauvegardees dans agence_data.txt)
//
//  Classes metier : Passager, Avion, Vol, Reservation, Billet, Paiement,
//                   Annulation  (+ la classe Agence qui orchestre le tout)
//  Encapsulation : attributs prives, acces via methodes publiques/getters
//                  (la classe Agence est amie pour gerer listes et fichier).
// ============================================================================
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// ----------------------------------------------------------------------------
//  Enumerations
// ----------------------------------------------------------------------------
enum class StatutVol { PREVU, ANNULE, TERMINE };
enum class ClasseVol { ECONOMIQUE, AFFAIRES };
enum class StatutReservation { CONFIRMEE, EN_ATTENTE, ANNULEE };
enum class StatutBillet { VALIDE, UTILISE, ANNULE };
enum class ModePaiement { MTN_MOMO, MOOV_MONEY, ESPECES, CARTE };
enum class StatutPaiement { PAYE, EN_ATTENTE, ECHOUE, REMBOURSE };

string texte(StatutVol s) {
    switch (s) {
        case StatutVol::PREVU: return "PREVU";
        case StatutVol::ANNULE: return "ANNULE";
        default: return "TERMINE";
    }
}
string texte(ClasseVol c) { return c == ClasseVol::ECONOMIQUE ? "ECONOMIQUE" : "AFFAIRES"; }
string texte(StatutReservation s) {
    switch (s) {
        case StatutReservation::CONFIRMEE: return "CONFIRMEE";
        case StatutReservation::EN_ATTENTE: return "EN_ATTENTE";
        default: return "ANNULEE";
    }
}
string texte(StatutBillet s) {
    switch (s) {
        case StatutBillet::VALIDE: return "VALIDE";
        case StatutBillet::UTILISE: return "UTILISE";
        default: return "ANNULE";
    }
}
string texte(ModePaiement m) {
    switch (m) {
        case ModePaiement::MTN_MOMO: return "MTN MoMo";
        case ModePaiement::MOOV_MONEY: return "Moov Money";
        case ModePaiement::ESPECES: return "Especes";
        default: return "Carte";
    }
}
string texte(StatutPaiement s) {
    switch (s) {
        case StatutPaiement::PAYE: return "PAYE";
        case StatutPaiement::EN_ATTENTE: return "EN_ATTENTE";
        case StatutPaiement::ECHOUE: return "ECHOUE";
        default: return "REMBOURSE";
    }
}

// ----------------------------------------------------------------------------
//  Utilitaires de saisie / dates / fichiers
// ----------------------------------------------------------------------------
string lireLigne(const string& invite) {
    cout << invite;
    string s;
    if (!getline(cin, s)) {
        cout << "\nFin de l'entree. Au revoir.\n";
        exit(0);
    }
    for (char& c : s)
        if (c == '|') c = '/';  // '|' est le separateur du fichier de donnees
    return s;
}

int lireEntier(const string& invite) {
    while (true) {
        string s = lireLigne(invite);
        try {
            size_t pos = 0;
            int v = stoi(s, &pos);
            if (pos == s.size()) return v;
        } catch (...) {
        }
        cout << "  Entree invalide, recommencez.\n";
    }
}

double lireReel(const string& invite) {
    while (true) {
        string s = lireLigne(invite);
        try {
            size_t pos = 0;
            double v = stod(s, &pos);
            if (pos == s.size() && v >= 0) return v;
        } catch (...) {
        }
        cout << "  Entree invalide, recommencez.\n";
    }
}

string dateDuJour() {
    time_t t = time(nullptr);
    tm* l = localtime(&t);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", l);
    return buf;
}

vector<string> decouper(const string& s, char sep) {
    vector<string> parts;
    string cur;
    stringstream ss(s);
    while (getline(ss, cur, sep)) parts.push_back(cur);
    if (!s.empty() && s.back() == sep) parts.push_back("");
    return parts;
}

// ============================================================================
//  CLASSE Passager
// ============================================================================
class Passager {
private:
    friend class Agence;  // l'Agence gere les listes et la persistance
    int idPassager = 0;
    string nom, prenom, dateNaissance, numeroPasseport;  // numeroPasseport : unique
    string telephone, email, nationalite;

public:
    int getId() const { return idPassager; }
    const string& getNom() const { return nom; }
    const string& getPrenom() const { return prenom; }
    const string& getNumeroPasseport() const { return numeroPasseport; }

    void creerPassager(int id, const string& passeport) {
        idPassager = id;
        numeroPasseport = passeport;
        nom = lireLigne("  Nom : ");
        prenom = lireLigne("  Prenom : ");
        dateNaissance = lireLigne("  Date de naissance (AAAA-MM-JJ) : ");
        telephone = lireLigne("  Telephone : ");
        email = lireLigne("  Email : ");
        nationalite = lireLigne("  Nationalite : ");
    }

    void modifierPassager() {
        cout << "  (laisser vide pour conserver la valeur actuelle)\n";
        string v;
        v = lireLigne("  Nom [" + nom + "] : ");                      if (!v.empty()) nom = v;
        v = lireLigne("  Prenom [" + prenom + "] : ");                if (!v.empty()) prenom = v;
        v = lireLigne("  Date naissance [" + dateNaissance + "] : "); if (!v.empty()) dateNaissance = v;
        v = lireLigne("  Telephone [" + telephone + "] : ");          if (!v.empty()) telephone = v;
        v = lireLigne("  Email [" + email + "] : ");                  if (!v.empty()) email = v;
        v = lireLigne("  Nationalite [" + nationalite + "] : ");      if (!v.empty()) nationalite = v;
    }

    void afficherInfos() const {
        cout << "  #" << idPassager << " " << nom << " " << prenom << " | Passeport : " << numeroPasseport
             << " | Ne(e) le " << dateNaissance << " | Tel : " << telephone << " | " << email
             << " | " << nationalite << "\n";
    }

    static Passager* rechercherParPasseport(vector<Passager>& liste, const string& passeport) {
        for (auto& p : liste)
            if (p.numeroPasseport == passeport) return &p;
        return nullptr;
    }
};

// ============================================================================
//  CLASSE Avion
// ============================================================================
class Avion {
private:
    friend class Agence;  // l'Agence gere les listes et la persistance
    int idAvion = 0;
    string modele;  // ex : Boeing 737
    int capaciteTotale = 0;
    string compagnie;

public:
    int getId() const { return idAvion; }
    int getCapaciteTotale() const { return capaciteTotale; }
    const string& getModele() const { return modele; }

    void creerAvion(int id) {
        idAvion = id;
        modele = lireLigne("  Modele (ex : Boeing 737) : ");
        capaciteTotale = lireEntier("  Capacite totale (nombre de sieges) : ");
        compagnie = lireLigne("  Compagnie : ");
    }

    bool verifierDisponibilite(int siegesOccupes) const { return siegesOccupes < capaciteTotale; }

    void afficherAvion() const {
        cout << "  #" << idAvion << " " << modele << " | " << capaciteTotale << " sieges | " << compagnie << "\n";
    }
};

// ============================================================================
//  CLASSE Vol
// ============================================================================
class Vol {
private:
    friend class Agence;  // l'Agence gere les listes et la persistance
    int idVol = 0;
    string numeroVol;  // ex : VC-101
    string villeDepart = "Cotonou", villeArrivee;
    string dateDepart, dateArrivee;
    double prixBase = 0;
    int placesDisponibles = 0;
    StatutVol statut = StatutVol::PREVU;
    int idAvion = 0;
    set<int> siegesOccupes;  // sieges deja pris : empeche la double reservation

public:
    int getId() const { return idVol; }
    const string& getNumeroVol() const { return numeroVol; }
    double getPrixBase() const { return prixBase; }
    int getPlacesDisponibles() const { return placesDisponibles; }
    StatutVol getStatut() const { return statut; }

    void creerVol(int id, const Avion& avion) {
        idVol = id;
        idAvion = avion.getId();
        numeroVol = lireLigne("  Numero de vol (ex : VC-101) : ");
        string v = lireLigne("  Ville de depart [Cotonou] : ");
        if (!v.empty()) villeDepart = v;
        villeArrivee = lireLigne("  Ville d'arrivee : ");
        dateDepart = lireLigne("  Date/heure de depart (AAAA-MM-JJ HH:MM) : ");
        dateArrivee = lireLigne("  Date/heure d'arrivee (AAAA-MM-JJ HH:MM) : ");
        prixBase = lireReel("  Prix de base (FCFA, classe economique) : ");
        statut = StatutVol::PREVU;
        placesDisponibles = avion.getCapaciteTotale();
    }

    void modifierVol() {
        cout << "  (laisser vide pour conserver la valeur actuelle)\n";
        string v;
        v = lireLigne("  Date depart [" + dateDepart + "] : ");   if (!v.empty()) dateDepart = v;
        v = lireLigne("  Date arrivee [" + dateArrivee + "] : "); if (!v.empty()) dateArrivee = v;
        v = lireLigne("  Prix de base [" + to_string((long long)prixBase) + "] : ");
        if (!v.empty()) {
            try { prixBase = stod(v); } catch (...) { cout << "  Prix invalide, ignore.\n"; }
        }
    }

    void annulerVol() { statut = StatutVol::ANNULE; }

    int calculerPlacesRestantes(int capacite) {
        placesDisponibles = capacite - (int)siegesOccupes.size();
        return placesDisponibles;
    }

    bool estComplet() const { return placesDisponibles <= 0; }

    void afficherVol() const {
        cout << "  #" << idVol << " " << numeroVol << " | " << villeDepart << " -> " << villeArrivee
             << " | " << dateDepart << " -> " << dateArrivee << " | " << fixed << setprecision(0) << prixBase
             << " FCFA | places libres : " << placesDisponibles << " | " << texte(statut) << "\n";
    }
};

// ============================================================================
//  CLASSE Reservation
// ============================================================================
class Reservation {
private:
    friend class Agence;  // l'Agence gere les listes et la persistance
    int idReservation = 0, idPassager = 0, idVol = 0;
    string dateReservation;
    ClasseVol classe = ClasseVol::ECONOMIQUE;
    int nombrePlaces = 0;
    vector<int> sieges;  // un numero de siege par place reservee
    StatutReservation statut = StatutReservation::EN_ATTENTE;
    double prixTotal = 0;

public:
    int getId() const { return idReservation; }
    double getPrixTotal() const { return prixTotal; }
    StatutReservation getStatut() const { return statut; }

    void creerReservation(int id, int passager, int vol, ClasseVol c, const vector<int>& siegesChoisis) {
        idReservation = id;
        idPassager = passager;
        idVol = vol;
        classe = c;
        sieges = siegesChoisis;
        nombrePlaces = (int)siegesChoisis.size();
        dateReservation = dateDuJour();
        statut = StatutReservation::EN_ATTENTE;
    }

    // Classe Affaires = 2 x le prix de base
    void calculerPrixTotal(double prixBase) {
        double coef = (classe == ClasseVol::AFFAIRES) ? 2.0 : 1.0;
        prixTotal = prixBase * coef * nombrePlaces;
    }

    void confirmerReservation() { statut = StatutReservation::CONFIRMEE; }
    void annulerReservation() { statut = StatutReservation::ANNULEE; }

    string siegesTexte() const {
        string s;
        for (size_t i = 0; i < sieges.size(); ++i) s += (i ? "," : "") + to_string(sieges[i]);
        return s;
    }

    void afficherReservation() const {
        cout << "  #" << idReservation << " | passager #" << idPassager << " | vol #" << idVol << " | "
             << texte(classe) << " | " << nombrePlaces << " place(s) [sieges " << siegesTexte() << "] | "
             << fixed << setprecision(0) << prixTotal << " FCFA | " << texte(statut) << " | " << dateReservation << "\n";
    }
};

// ============================================================================
//  CLASSE Billet
// ============================================================================
class Billet {
private:
    friend class Agence;  // l'Agence gere les listes et la persistance
    int idBillet = 0, idReservation = 0, siege = 0;
    string numeroBillet;  // unique
    string dateEmission;
    StatutBillet statut = StatutBillet::VALIDE;

public:
    const string& getNumeroBillet() const { return numeroBillet; }
    StatutBillet getStatut() const { return statut; }

    void genererBillet(int id, int reservation, int numSiege) {
        idBillet = id;
        idReservation = reservation;
        siege = numSiege;
        dateEmission = dateDuJour();
        statut = StatutBillet::VALIDE;
        numeroBillet = "BJ-" + to_string(100000 + id);
    }

    void imprimerBillet() const {
        cout << "  +------------------------------------+\n"
             << "  |  BILLET " << numeroBillet << "\n"
             << "  |  Reservation #" << idReservation << "   Siege : " << siege << "\n"
             << "  |  Emis le " << dateEmission << "   Statut : " << texte(statut) << "\n"
             << "  +------------------------------------+\n";
    }

    void validerBillet() {  // billet utilise a l'embarquement
        if (statut == StatutBillet::VALIDE) statut = StatutBillet::UTILISE;
    }
};

// ============================================================================
//  CLASSE Paiement
// ============================================================================
class Paiement {
private:
    friend class Agence;  // l'Agence gere les listes et la persistance
    int idPaiement = 0, idReservation = 0;
    double montant = 0;
    string datePaiement;
    ModePaiement modePaiement = ModePaiement::ESPECES;
    StatutPaiement statut = StatutPaiement::EN_ATTENTE;

public:
    double getMontant() const { return montant; }
    StatutPaiement getStatut() const { return statut; }

    void effectuerPaiement(int id, int reservation, double m, ModePaiement mode) {
        idPaiement = id;
        idReservation = reservation;
        montant = m;
        modePaiement = mode;
        datePaiement = dateDuJour();
        statut = StatutPaiement::PAYE;
    }

    bool verifierPaiement() const { return statut == StatutPaiement::PAYE; }
    void rembourser() { statut = StatutPaiement::REMBOURSE; }

    void genererRecu() const {
        cout << "  ---------------- RECU ----------------\n"
             << "  Paiement #" << idPaiement << "  (reservation #" << idReservation << ")\n"
             << "  Montant : " << fixed << setprecision(0) << montant << " FCFA\n"
             << "  Mode    : " << texte(modePaiement) << "\n"
             << "  Date    : " << datePaiement << "   Statut : " << texte(statut) << "\n"
             << "  --------------------------------------\n";
    }
};

// ============================================================================
//  CLASSE Annulation
// ============================================================================
class Annulation {
public:
    static constexpr double TAUX_FRAIS = 0.20;  // 20 % de frais si le client annule

private:
    friend class Agence;
    int idAnnulation = 0, idReservation = 0;
    string dateAnnulation, motif;
    double montantRembourse = 0, fraisAnnulation = 0;

public:
    double getMontantRembourse() const { return montantRembourse; }
    double getFraisAnnulation() const { return fraisAnnulation; }

    // Vol annule par la compagnie => remboursement integral
    void calculerRemboursement(double montantPaye, bool volAnnule) {
        fraisAnnulation = volAnnule ? 0.0 : montantPaye * TAUX_FRAIS;
        montantRembourse = montantPaye - fraisAnnulation;
    }

    void enregistrerAnnulation(int id, int reservation, const string& m) {
        idAnnulation = id;
        idReservation = reservation;
        motif = m;
        dateAnnulation = dateDuJour();
    }

    void afficherAnnulation() const {
        cout << "  #" << idAnnulation << " | reservation #" << idReservation << " | " << dateAnnulation
             << " | motif : " << motif << " | frais : " << fixed << setprecision(0) << fraisAnnulation
             << " FCFA | rembourse : " << montantRembourse << " FCFA\n";
    }
};

// ============================================================================
//  CLASSE Agence : gere les listes, les regles metier et la persistance
// ============================================================================
class Agence {
    vector<Passager> passagers;
    vector<Avion> avions;
    vector<Vol> vols;
    vector<Reservation> reservations;
    vector<Billet> billets;
    vector<Paiement> paiements;
    vector<Annulation> annulations;
    const string FICHIER = "agence_data.txt";

    template <class T>
    int prochainId(const vector<T>& v, int T::*champ) const {
        int m = 0;
        for (const auto& e : v) m = max(m, e.*champ);
        return m + 1;
    }

    Passager* trouverPassager(int id) { for (auto& p : passagers) if (p.idPassager == id) return &p; return nullptr; }
    Avion* trouverAvion(int id) { for (auto& a : avions) if (a.idAvion == id) return &a; return nullptr; }
    Vol* trouverVol(int id) { for (auto& v : vols) if (v.idVol == id) return &v; return nullptr; }
    Reservation* trouverReservation(int id) { for (auto& r : reservations) if (r.idReservation == id) return &r; return nullptr; }
    Paiement* paiementDe(int idRes) { for (auto& p : paiements) if (p.idReservation == idRes) return &p; return nullptr; }

    // Recalcule sieges occupes et places disponibles a partir des reservations actives
    void recalculer() {
        for (auto& v : vols) v.siegesOccupes.clear();
        for (const auto& r : reservations) {
            if (r.statut == StatutReservation::ANNULEE) continue;
            if (Vol* v = trouverVol(r.idVol))
                for (int s : r.sieges) v->siegesOccupes.insert(s);
        }
        for (auto& v : vols)
            if (Avion* a = trouverAvion(v.idAvion)) v.calculerPlacesRestantes(a->capaciteTotale);
    }

    // ---------------- Persistance ----------------
    void sauvegarder() const {
        ofstream f(FICHIER);
        f << fixed << setprecision(2);
        for (auto& p : passagers)
            f << "PAS|" << p.idPassager << '|' << p.nom << '|' << p.prenom << '|' << p.dateNaissance << '|'
              << p.numeroPasseport << '|' << p.telephone << '|' << p.email << '|' << p.nationalite << '\n';
        for (auto& a : avions)
            f << "AVI|" << a.idAvion << '|' << a.modele << '|' << a.capaciteTotale << '|' << a.compagnie << '\n';
        for (auto& v : vols)
            f << "VOL|" << v.idVol << '|' << v.numeroVol << '|' << v.villeDepart << '|' << v.villeArrivee << '|'
              << v.dateDepart << '|' << v.dateArrivee << '|' << v.prixBase << '|' << (int)v.statut << '|' << v.idAvion << '\n';
        for (auto& r : reservations)
            f << "RES|" << r.idReservation << '|' << r.idPassager << '|' << r.idVol << '|' << r.dateReservation << '|'
              << (int)r.classe << '|' << r.siegesTexte() << '|' << (int)r.statut << '|' << r.prixTotal << '\n';
        for (auto& b : billets)
            f << "BIL|" << b.idBillet << '|' << b.idReservation << '|' << b.numeroBillet << '|' << b.dateEmission << '|'
              << b.siege << '|' << (int)b.statut << '\n';
        for (auto& p : paiements)
            f << "PAI|" << p.idPaiement << '|' << p.idReservation << '|' << p.montant << '|' << p.datePaiement << '|'
              << (int)p.modePaiement << '|' << (int)p.statut << '\n';
        for (auto& a : annulations)
            f << "ANN|" << a.idAnnulation << '|' << a.idReservation << '|' << a.dateAnnulation << '|' << a.motif << '|'
              << a.montantRembourse << '|' << a.fraisAnnulation << '\n';
    }

    void charger() {
        ifstream f(FICHIER);
        if (!f) return;
        string ligne;
        while (getline(f, ligne)) {
            auto t = decouper(ligne, '|');
            if (t.empty()) continue;
            try {
                if (t[0] == "PAS" && t.size() >= 9) {
                    Passager p; p.idPassager = stoi(t[1]); p.nom = t[2]; p.prenom = t[3]; p.dateNaissance = t[4];
                    p.numeroPasseport = t[5]; p.telephone = t[6]; p.email = t[7]; p.nationalite = t[8];
                    passagers.push_back(p);
                } else if (t[0] == "AVI" && t.size() >= 5) {
                    Avion a; a.idAvion = stoi(t[1]); a.modele = t[2]; a.capaciteTotale = stoi(t[3]); a.compagnie = t[4];
                    avions.push_back(a);
                } else if (t[0] == "VOL" && t.size() >= 10) {
                    Vol v; v.idVol = stoi(t[1]); v.numeroVol = t[2]; v.villeDepart = t[3]; v.villeArrivee = t[4];
                    v.dateDepart = t[5]; v.dateArrivee = t[6]; v.prixBase = stod(t[7]);
                    v.statut = (StatutVol)stoi(t[8]); v.idAvion = stoi(t[9]);
                    vols.push_back(v);
                } else if (t[0] == "RES" && t.size() >= 9) {
                    Reservation r; r.idReservation = stoi(t[1]); r.idPassager = stoi(t[2]); r.idVol = stoi(t[3]);
                    r.dateReservation = t[4]; r.classe = (ClasseVol)stoi(t[5]);
                    for (auto& s : decouper(t[6], ',')) if (!s.empty()) r.sieges.push_back(stoi(s));
                    r.nombrePlaces = (int)r.sieges.size();
                    r.statut = (StatutReservation)stoi(t[7]); r.prixTotal = stod(t[8]);
                    reservations.push_back(r);
                } else if (t[0] == "BIL" && t.size() >= 7) {
                    Billet b; b.idBillet = stoi(t[1]); b.idReservation = stoi(t[2]); b.numeroBillet = t[3];
                    b.dateEmission = t[4]; b.siege = stoi(t[5]); b.statut = (StatutBillet)stoi(t[6]);
                    billets.push_back(b);
                } else if (t[0] == "PAI" && t.size() >= 7) {
                    Paiement p; p.idPaiement = stoi(t[1]); p.idReservation = stoi(t[2]); p.montant = stod(t[3]);
                    p.datePaiement = t[4]; p.modePaiement = (ModePaiement)stoi(t[5]); p.statut = (StatutPaiement)stoi(t[6]);
                    paiements.push_back(p);
                } else if (t[0] == "ANN" && t.size() >= 7) {
                    Annulation a; a.idAnnulation = stoi(t[1]); a.idReservation = stoi(t[2]); a.dateAnnulation = t[3];
                    a.motif = t[4]; a.montantRembourse = stod(t[5]); a.fraisAnnulation = stod(t[6]);
                    annulations.push_back(a);
                }
            } catch (...) {
                cout << "  Ligne ignoree (donnees corrompues) : " << ligne << "\n";
            }
        }
        recalculer();
    }

    // ---------------- Regles metier ----------------
    // Annule une reservation : libere les sieges, annule les billets, rembourse si paye
    void annulerReservationInterne(Reservation& r, const string& motif, bool volAnnule) {
        Annulation a;
        a.enregistrerAnnulation(prochainId(annulations, &Annulation::idAnnulation), r.idReservation, motif);

        Paiement* p = paiementDe(r.idReservation);
        if (p && p->verifierPaiement()) {
            a.calculerRemboursement(p->montant, volAnnule);
            p->rembourser();
        } else {
            a.calculerRemboursement(0, volAnnule);
        }
        for (auto& b : billets)
            if (b.idReservation == r.idReservation) b.statut = StatutBillet::ANNULE;

        r.annulerReservation();
        annulations.push_back(a);
        recalculer();  // libere les sieges
        a.afficherAnnulation();
    }

    bool aReservationActive(int idPassager) const {
        for (const auto& r : reservations)
            if (r.idPassager == idPassager && r.statut != StatutReservation::ANNULEE) return true;
        return false;
    }

public:
    Agence() { charger(); }

    // ====================== PASSAGERS ======================
    void creerPassager() {
        string passeport = lireLigne("  Numero de passeport : ");
        if (passeport.empty()) { cout << "  Passeport obligatoire.\n"; return; }
        if (Passager::rechercherParPasseport(passagers, passeport)) {
            cout << "  Erreur : ce passeport existe deja (un passager = un passeport unique).\n";
            return;
        }
        Passager p;
        p.creerPassager(prochainId(passagers, &Passager::idPassager), passeport);
        passagers.push_back(p);
        sauvegarder();
        cout << "  Passager cree (id " << p.idPassager << ").\n";
    }

    void modifierPassager() {
        Passager* p = Passager::rechercherParPasseport(passagers, lireLigne("  Passeport du passager : "));
        if (!p) { cout << "  Passager introuvable.\n"; return; }
        p->modifierPassager();
        sauvegarder();
        cout << "  Passager modifie.\n";
    }

    void supprimerPassager() {
        Passager* p = Passager::rechercherParPasseport(passagers, lireLigne("  Passeport du passager : "));
        if (!p) { cout << "  Passager introuvable.\n"; return; }
        if (aReservationActive(p->idPassager)) {
            cout << "  Suppression impossible : ce passager a des reservations actives.\n";
            return;
        }
        int id = p->idPassager;
        passagers.erase(remove_if(passagers.begin(), passagers.end(),
                                  [id](const Passager& x) { return x.idPassager == id; }), passagers.end());
        sauvegarder();
        cout << "  Passager supprime.\n";
    }

    void rechercherPassager() {
        Passager* p = Passager::rechercherParPasseport(passagers, lireLigne("  Passeport recherche : "));
        if (p) p->afficherInfos(); else cout << "  Aucun passager avec ce passeport.\n";
    }

    void listerPassagers() const {
        if (passagers.empty()) cout << "  Aucun passager.\n";
        for (const auto& p : passagers) p.afficherInfos();
    }

    // ====================== AVIONS ======================
    void creerAvion() {
        Avion a;
        a.creerAvion(prochainId(avions, &Avion::idAvion));
        if (a.capaciteTotale <= 0) { cout << "  Capacite invalide.\n"; return; }
        avions.push_back(a);
        sauvegarder();
        cout << "  Avion cree (id " << a.idAvion << ").\n";
    }

    void listerAvions() const {
        if (avions.empty()) cout << "  Aucun avion.\n";
        for (const auto& a : avions) a.afficherAvion();
    }

    // ====================== VOLS ======================
    void creerVol() {
        if (avions.empty()) { cout << "  Creez d'abord un avion.\n"; return; }
        listerAvions();
        Avion* a = trouverAvion(lireEntier("  Id de l'avion assurant le vol : "));
        if (!a) { cout << "  Avion introuvable.\n"; return; }
        Vol v;
        v.creerVol(prochainId(vols, &Vol::idVol), *a);
        vols.push_back(v);
        sauvegarder();
        cout << "  Vol cree (id " << v.idVol << ").\n";
    }

    void modifierVol() {
        Vol* v = trouverVol(lireEntier("  Id du vol : "));
        if (!v) { cout << "  Vol introuvable.\n"; return; }
        v->modifierVol();
        sauvegarder();
        cout << "  Vol modifie.\n";
    }

    void annulerVol() {
        Vol* v = trouverVol(lireEntier("  Id du vol a annuler : "));
        if (!v) { cout << "  Vol introuvable.\n"; return; }
        if (v->statut != StatutVol::PREVU) { cout << "  Seul un vol PREVU peut etre annule.\n"; return; }
        v->annulerVol();
        cout << "  Vol annule. Annulation des reservations liees (remboursement integral) :\n";
        for (auto& r : reservations)
            if (r.idVol == v->idVol && r.statut != StatutReservation::ANNULEE)
                annulerReservationInterne(r, "Vol annule par la compagnie", true);
        recalculer();
        sauvegarder();
    }

    void terminerVol() {
        Vol* v = trouverVol(lireEntier("  Id du vol a marquer TERMINE : "));
        if (!v) { cout << "  Vol introuvable.\n"; return; }
        if (v->statut != StatutVol::PREVU) { cout << "  Seul un vol PREVU peut etre termine.\n"; return; }
        v->statut = StatutVol::TERMINE;
        sauvegarder();
        cout << "  Vol marque TERMINE.\n";
    }

    void listerVols() const {
        if (vols.empty()) cout << "  Aucun vol.\n";
        for (const auto& v : vols) v.afficherVol();
    }

    // ====================== RESERVATIONS ======================
    void creerReservation() {
        Passager* p = Passager::rechercherParPasseport(passagers, lireLigne("  Passeport du passager : "));
        if (!p) { cout << "  Passager introuvable (creez-le d'abord).\n"; return; }

        cout << "  Vols disponibles :\n";
        bool aucun = true;
        for (const auto& v : vols)
            if (v.statut == StatutVol::PREVU && !v.estComplet()) { v.afficherVol(); aucun = false; }
        if (aucun) { cout << "  Aucun vol disponible.\n"; return; }

        Vol* v = trouverVol(lireEntier("  Id du vol : "));
        if (!v || v->statut != StatutVol::PREVU) { cout << "  Vol introuvable ou non reservable.\n"; return; }
        if (v->estComplet()) { cout << "  Vol complet.\n"; return; }
        Avion* a = trouverAvion(v->idAvion);
        if (!a) { cout << "  Avion du vol introuvable.\n"; return; }

        int choixClasse = lireEntier("  Classe (1 = Economique, 2 = Affaires) : ");
        ClasseVol classe = (choixClasse == 2) ? ClasseVol::AFFAIRES : ClasseVol::ECONOMIQUE;

        int nb = lireEntier("  Nombre de places : ");
        if (nb <= 0 || nb > v->placesDisponibles) {
            cout << "  Nombre de places invalide (disponibles : " << v->placesDisponibles << ").\n";
            return;
        }

        cout << "  Sieges deja occupes :";
        if (v->siegesOccupes.empty()) cout << " aucun";
        for (int s : v->siegesOccupes) cout << " " << s;
        cout << "\n";

        vector<int> choisis;
        while ((int)choisis.size() < nb) {
            int s = lireEntier("  Siege n°" + to_string(choisis.size() + 1) + " (1-" + to_string(a->capaciteTotale) + ") : ");
            if (s < 1 || s > a->capaciteTotale)
                cout << "  Siege inexistant.\n";
            else if (v->siegesOccupes.count(s))
                cout << "  Siege deja reserve ! Choisissez-en un autre.\n";
            else if (find(choisis.begin(), choisis.end(), s) != choisis.end())
                cout << "  Siege deja choisi dans cette reservation.\n";
            else
                choisis.push_back(s);
        }

        Reservation r;
        r.creerReservation(prochainId(reservations, &Reservation::idReservation), p->idPassager, v->idVol, classe, choisis);
        r.calculerPrixTotal(v->prixBase);
        reservations.push_back(r);
        recalculer();
        sauvegarder();
        cout << "  Reservation #" << r.idReservation << " creee (EN_ATTENTE de paiement). Total : "
             << fixed << setprecision(0) << r.prixTotal << " FCFA\n";
    }

    void listerReservations() const {
        if (reservations.empty()) cout << "  Aucune reservation.\n";
        for (const auto& r : reservations) r.afficherReservation();
    }

    // ====================== PAIEMENTS ======================
    void payerReservation() {
        Reservation* r = trouverReservation(lireEntier("  Id de la reservation a payer : "));
        if (!r) { cout << "  Reservation introuvable.\n"; return; }
        if (r->statut == StatutReservation::ANNULEE) { cout << "  Reservation annulee.\n"; return; }
        if (paiementDe(r->idReservation)) { cout << "  Cette reservation est deja payee (1 paiement = 1 reservation).\n"; return; }

        cout << "  Montant a payer : " << fixed << setprecision(0) << r->prixTotal << " FCFA\n"
             << "  Mode : 1 = MTN MoMo, 2 = Moov Money, 3 = Especes, 4 = Carte\n";
        int m = lireEntier("  Choix : ");
        if (m < 1 || m > 4) { cout << "  Mode invalide.\n"; return; }

        Paiement p;
        p.effectuerPaiement(prochainId(paiements, &Paiement::idPaiement), r->idReservation, r->prixTotal, (ModePaiement)(m - 1));
        paiements.push_back(p);
        r->confirmerReservation();

        cout << "  Billets emis :\n";
        for (int siege : r->sieges) {
            Billet b;
            b.genererBillet(prochainId(billets, &Billet::idBillet), r->idReservation, siege);
            billets.push_back(b);
            b.imprimerBillet();
        }
        p.genererRecu();
        sauvegarder();
    }

    void listerPaiements() const {
        if (paiements.empty()) cout << "  Aucun paiement.\n";
        for (const auto& p : paiements) p.genererRecu();
    }

    // ====================== ANNULATIONS ======================
    void annulerReservation() {
        Reservation* r = trouverReservation(lireEntier("  Id de la reservation a annuler : "));
        if (!r) { cout << "  Reservation introuvable.\n"; return; }
        if (r->statut == StatutReservation::ANNULEE) { cout << "  Deja annulee.\n"; return; }
        string motif = lireLigne("  Motif : ");
        cout << "  (frais d'annulation : " << (int)(Annulation::TAUX_FRAIS * 100) << " % du montant paye)\n";
        annulerReservationInterne(*r, motif, false);
        sauvegarder();
    }

    void listerAnnulations() const {
        if (annulations.empty()) cout << "  Aucune annulation.\n";
        for (const auto& a : annulations) a.afficherAnnulation();
    }

    void validerBillet() {
        string num = lireLigne("  Numero de billet (ex : BJ-100001) : ");
        for (auto& b : billets)
            if (b.numeroBillet == num) {
                if (b.statut != StatutBillet::VALIDE) { cout << "  Billet non valide (" << texte(b.statut) << ").\n"; return; }
                b.validerBillet();
                sauvegarder();
                cout << "  Billet valide a l'embarquement.\n";
                return;
            }
        cout << "  Billet introuvable.\n";
    }
};

// ============================================================================
//  Menus
// ============================================================================
void menuPassagers(Agence& ag) {
    while (true) {
        cout << "\n--- PASSAGERS ---\n1. Creer  2. Modifier  3. Supprimer  4. Rechercher (passeport)  5. Lister  0. Retour\n";
        switch (lireEntier("Choix : ")) {
            case 1: ag.creerPassager(); break;
            case 2: ag.modifierPassager(); break;
            case 3: ag.supprimerPassager(); break;
            case 4: ag.rechercherPassager(); break;
            case 5: ag.listerPassagers(); break;
            case 0: return;
            default: cout << "Choix invalide.\n";
        }
    }
}

void menuVols(Agence& ag) {
    while (true) {
        cout << "\n--- AVIONS & VOLS ---\n1. Creer avion  2. Lister avions  3. Creer vol  4. Modifier vol\n"
                "5. Annuler vol  6. Marquer vol termine  7. Lister vols  0. Retour\n";
        switch (lireEntier("Choix : ")) {
            case 1: ag.creerAvion(); break;
            case 2: ag.listerAvions(); break;
            case 3: ag.creerVol(); break;
            case 4: ag.modifierVol(); break;
            case 5: ag.annulerVol(); break;
            case 6: ag.terminerVol(); break;
            case 7: ag.listerVols(); break;
            case 0: return;
            default: cout << "Choix invalide.\n";
        }
    }
}

void menuReservations(Agence& ag) {
    while (true) {
        cout << "\n--- RESERVATIONS & PAIEMENTS ---\n1. Nouvelle reservation  2. Lister reservations  3. Payer une reservation\n"
                "4. Lister paiements  5. Valider un billet (embarquement)  0. Retour\n";
        switch (lireEntier("Choix : ")) {
            case 1: ag.creerReservation(); break;
            case 2: ag.listerReservations(); break;
            case 3: ag.payerReservation(); break;
            case 4: ag.listerPaiements(); break;
            case 5: ag.validerBillet(); break;
            case 0: return;
            default: cout << "Choix invalide.\n";
        }
    }
}

void menuAnnulations(Agence& ag) {
    while (true) {
        cout << "\n--- ANNULATIONS ---\n1. Annuler une reservation  2. Historique des annulations  0. Retour\n";
        switch (lireEntier("Choix : ")) {
            case 1: ag.annulerReservation(); break;
            case 2: ag.listerAnnulations(); break;
            case 0: return;
            default: cout << "Choix invalide.\n";
        }
    }
}

int main() {
    Agence agence;
    cout << "=====================================\n"
            "   AGENCE DE VOYAGE - COTONOU (BJ)\n"
            "=====================================\n";
    while (true) {
        cout << "\n=== MENU PRINCIPAL ===\n1. Passagers\n2. Avions et vols\n3. Reservations et paiements\n4. Annulations\n0. Quitter\n";
        switch (lireEntier("Choix : ")) {
            case 1: menuPassagers(agence); break;
            case 2: menuVols(agence); break;
            case 3: menuReservations(agence); break;
            case 4: menuAnnulations(agence); break;
            case 0: cout << "Donnees sauvegardees. Au revoir !\n"; return 0;
            default: cout << "Choix invalide.\n";
        }
    }
}
