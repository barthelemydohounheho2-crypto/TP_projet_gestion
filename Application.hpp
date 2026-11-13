#pragma once
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "Services.hpp"

class FinDeSaisie : public std::runtime_error {
public:
    FinDeSaisie() : std::runtime_error("Fin de saisie") {}
};

// Couche présentation : saisie/affichage et orchestration des cas d'utilisation.
// Toute la logique métier reste dans les services.
class Application {
public:
    Application()
        : reservations_(passagers_, vols_),
          paiements_(reservations_, vols_, std::make_unique<PolitiqueStandard>()) {}

    void executer() {
        afficherMenu("MENU PRINCIPAL", {
            {"Gestion des passagers",            [this] { menuPassagers(); }},
            {"Gestion des vols",                 [this] { menuVols(); }},
            {"Gestion des réservations",         [this] { menuReservations(); }},
            {"Paiements et annulations",         [this] { menuPaiements(); }},
            {"Charger des données de démonstration", [this] { chargerDemo(); }},
        });
        std::cout << "Au revoir.\n";
    }

private:
    struct Entree { std::string libelle; std::function<void()> action; };

    // ----- Saisie ----------------------------------------------------------
    static std::string lireLigne(const std::string& invite) {
        std::cout << invite << " : ";
        std::string s;
        if (!std::getline(std::cin, s)) throw FinDeSaisie();
        return s;
    }
    static int lireEntier(const std::string& invite) {
        for (;;) {
            try { return std::stoi(lireLigne(invite)); }
            catch (const FinDeSaisie&) { throw; }
            catch (...) { std::cout << "Entrez un nombre entier.\n"; }
        }
    }
    static double lireReel(const std::string& invite) {
        for (;;) {
            try { return std::stod(lireLigne(invite)); }
            catch (const FinDeSaisie&) { throw; }
            catch (...) { std::cout << "Entrez un nombre.\n"; }
        }
    }

    // ----- Moteur de menu générique ---------------------------------------
    static void afficherMenu(const std::string& titre, const std::vector<Entree>& entrees) {
        for (;;) {
            std::cout << "\n=== " << titre << " ===\n";
            for (std::size_t i = 0; i < entrees.size(); ++i)
                std::cout << "  " << (i + 1) << ". " << entrees[i].libelle << "\n";
            std::cout << "  0. Retour / Quitter\n";
            const int choix = lireEntier("Votre choix");
            if (choix == 0) return;
            if (choix < 1 || choix > static_cast<int>(entrees.size())) {
                std::cout << "Choix invalide.\n";
                continue;
            }
            try { entrees[choix - 1].action(); }
            catch (const ErreurMetier& e) { std::cout << "[Erreur] " << e.what() << "\n"; }
        }
    }

    // ----- Passagers -------------------------------------------------------
    void menuPassagers() {
        afficherMenu("PASSAGERS", {
            {"Ajouter un passager", [this] {
                const std::string nom = lireLigne("Nom"), prenom = lireLigne("Prénom");
                const std::string pass = lireLigne("N° de passeport"), tel = lireLigne("Téléphone");
                std::cout << "Créé : " << passagers_.ajouter(nom, prenom, pass, tel).resume() << "\n";
            }},
            {"Lister les passagers", [this] {
                if (passagers_.tous().empty()) std::cout << "Aucun passager.\n";
                for (const auto& p : passagers_.tous()) std::cout << "  " << p.second.resume() << "\n";
            }},
            {"Modifier le téléphone", [this] {
                Passager& p = passagers_.obtenir(lireEntier("ID du passager"));
                p.changerTelephone(lireLigne("Nouveau téléphone"));
                std::cout << "Mis à jour : " << p.resume() << "\n";
            }},
            {"Supprimer un passager", [this] {
                const int id = lireEntier("ID du passager");
                passagers_.obtenir(id);
                if (reservations_.aDesReservationsActives(id))
                    throw ErreurMetier("Suppression impossible : réservations actives.");
                passagers_.supprimer(id);
                std::cout << "Passager supprimé.\n";
            }},
        });
    }

    // ----- Vols ------------------------------------------------------------
    void menuVols() {
        afficherMenu("VOLS", {
            {"Ajouter un vol", [this] {
                const std::string code = lireLigne("Code du vol"), o = lireLigne("Origine");
                const std::string d = lireLigne("Destination");
                const DateHeure dep = DateHeure::depuisTexte(lireLigne("Départ (JJ/MM/AAAA HH:MM)"));
                const DateHeure arr = DateHeure::depuisTexte(lireLigne("Arrivée (JJ/MM/AAAA HH:MM)"));
                const int cap = lireEntier("Capacité");
                const double prix = lireReel("Prix de base (FCFA)");
                std::cout << "Créé : " << vols_.ajouter(code, o, d, dep, arr, cap, prix).resume() << "\n";
            }},
            {"Lister les vols", [this] { afficherVols(vols_.rechercher("", "")); }},
            {"Rechercher (origine / destination)", [this] {
                const std::string o = lireLigne("Origine (vide = toutes)");
                const std::string d = lireLigne("Destination (vide = toutes)");
                afficherVols(vols_.rechercher(o, d));
            }},
            {"Annuler un vol (remboursement intégral)", [this] {
                const std::string code = lireLigne("Code du vol");
                vols_.obtenir(code).annuler();
                const int n = paiements_.annulerReservationsDuVol(code);
                std::cout << "Vol annulé ; " << n << " réservation(s) annulée(s) et remboursée(s).\n";
            }},
        });
    }
    static void afficherVols(const std::vector<const Vol*>& liste) {
        if (liste.empty()) std::cout << "Aucun vol.\n";
        for (const Vol* v : liste) std::cout << "  " << v->resume() << "\n";
    }

    // ----- Réservations ----------------------------------------------------
    void menuReservations() {
        afficherMenu("RÉSERVATIONS", {
            {"Créer une réservation", [this] {
                const int idP = lireEntier("ID du passager");
                const std::string code = lireLigne("Code du vol");
                const int c = lireEntier("Classe (1 = Économique, 2 = Affaires)");
                if (c != 1 && c != 2) throw ErreurMetier("Classe invalide.");
                const int id = reservations_.creer(idP, code, c == 1 ? ClasseCabine::ECONOMIQUE
                                                                      : ClasseCabine::AFFAIRES);
                std::cout << "Créée : " << reservations_.obtenir(id).resume()
                          << "\n  -> À payer via « Paiements et annulations ».\n";
            }},
            {"Lister les réservations", [this] {
                if (reservations_.toutes().empty()) std::cout << "Aucune réservation.\n";
                for (const auto& r : reservations_.toutes()) std::cout << "  " << r.second.resume() << "\n";
            }},
            {"Réservations d'un passager", [this] {
                const int idP = lireEntier("ID du passager");
                bool trouve = false;
                for (const auto& r : reservations_.toutes())
                    if (r.second.idPassager() == idP) { std::cout << "  " << r.second.resume() << "\n"; trouve = true; }
                if (!trouve) std::cout << "Aucune réservation.\n";
            }},
        });
    }

    // ----- Paiements et annulations ----------------------------------------
    void menuPaiements() {
        afficherMenu("PAIEMENTS ET ANNULATIONS", {
            {"Payer une réservation", [this] { payer(); }},
            {"Annuler une réservation", [this] {
                const int id = lireEntier("ID de la réservation");
                const double remb = paiements_.annuler(id);
                std::cout << "Réservation annulée. Remboursement : " << std::fixed
                          << std::setprecision(0) << remb << " FCFA\n";
            }},
            {"Total encaissé (net des remboursements)", [this] {
                std::cout << "Total : " << std::fixed << std::setprecision(0)
                          << paiements_.totalEncaisse() << " FCFA\n";
            }},
        });
    }
    void payer() {
        const int id = lireEntier("ID de la réservation");
        const double du = reservations_.obtenir(id).montant();
        std::cout << "Montant dû : " << std::fixed << std::setprecision(0) << du << " FCFA\n";
        const int mode = lireEntier("Mode (1 = Carte, 2 = Mobile Money, 3 = Espèces)");
        std::unique_ptr<Paiement> p;     // polymorphisme : on manipule l'abstraction
        switch (mode) {
            case 1: { const std::string n = lireLigne("N° de carte (16 chiffres)");
                      p = std::make_unique<PaiementCarte>(du, n, lireLigne("Titulaire")); break; }
            case 2: { const std::string op = lireLigne("Opérateur (MTN, Moov...)");
                      p = std::make_unique<PaiementMobileMoney>(du, op, lireLigne("Numéro")); break; }
            case 3: p = std::make_unique<PaiementEspeces>(du, lireLigne("Nom du caissier")); break;
            default: throw ErreurMetier("Mode de paiement invalide.");
        }
        paiements_.payer(id, std::move(p));
        std::cout << "Paiement accepté : " << reservations_.obtenir(id).resume() << "\n";
    }

    // ----- Démo ------------------------------------------------------------
    void chargerDemo() {
        const DateHeure now = DateHeure::maintenant();
        passagers_.ajouter("Houngbo", "Marc", "BJ123456", "97000001");
        passagers_.ajouter("Sossou", "Awa", "BJ654321", "96000002");
        vols_.ajouter("AF700", "Cotonou", "Paris", now.plusHeures(72), now.plusHeures(79), 3, 350000);
        vols_.ajouter("ET510", "Cotonou", "Addis-Abeba", now.plusHeures(30), now.plusHeures(36), 2, 280000);
        std::cout << "Données de démonstration chargées (2 passagers, 2 vols).\n";
    }

    GestionPassagers passagers_;
    GestionVols vols_;
    GestionReservations reservations_;
    GestionPaiements paiements_;
};
