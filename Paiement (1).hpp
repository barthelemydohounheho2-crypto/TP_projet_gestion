#pragma once
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include "Utils.hpp"

enum class StatutPaiement { EFFECTUE, REMBOURSE, REMBOURSE_PARTIEL, NON_REMBOURSE };

inline const char* libelle(StatutPaiement s) {
    switch (s) {
        case StatutPaiement::EFFECTUE:          return "Effectué";
        case StatutPaiement::REMBOURSE:         return "Remboursé";
        case StatutPaiement::REMBOURSE_PARTIEL: return "Remboursé partiellement";
        default:                                return "Non remboursé";
    }
}

// ---------------------------------------------------------------------------
// Classe abstraite : le code client ne dépend que de cette interface.
// ---------------------------------------------------------------------------
class Paiement {
public:
    explicit Paiement(double montant) : montant_(montant), date_(DateHeure::maintenant()) {
        if (montant_ <= 0) throw ErreurMetier("Le montant du paiement doit être positif.");
    }
    virtual ~Paiement() = default;

    virtual std::string mode() const = 0;     // ex. "Carte bancaire"
    virtual std::string details() const = 0;  // coordonnées masquées

    double montant() const { return montant_; }
    double montantRembourse() const { return rembourse_; }
    StatutPaiement statut() const { return statut_; }

    void rembourser(double m) {
        if (statut_ != StatutPaiement::EFFECTUE) throw ErreurMetier("Paiement déjà traité.");
        if (m < 0 || m > montant_) throw ErreurMetier("Montant de remboursement invalide.");
        rembourse_ = m;
        statut_ = (m == 0) ? StatutPaiement::NON_REMBOURSE
                : (m == montant_) ? StatutPaiement::REMBOURSE
                                  : StatutPaiement::REMBOURSE_PARTIEL;
    }

    std::string resume() const {
        std::ostringstream os;
        os << mode() << " (" << details() << ") " << std::fixed << std::setprecision(0)
           << montant_ << " FCFA le " << date_.texte() << " [" << libelle(statut_) << "]";
        if (rembourse_ > 0) os << " remboursé: " << rembourse_ << " FCFA";
        return os.str();
    }

private:
    double montant_;
    double rembourse_ = 0;
    DateHeure date_;
    StatutPaiement statut_ = StatutPaiement::EFFECTUE;
};

class PaiementCarte : public Paiement {
public:
    PaiementCarte(double montant, const std::string& numero, std::string titulaire)
        : Paiement(montant), titulaire_(std::move(titulaire)) {
        const std::string n = chiffresSeulement(numero);
        if (n.size() != 16) throw ErreurMetier("Le numéro de carte doit comporter 16 chiffres.");
        quatreDerniers_ = n.substr(12);   // on ne conserve jamais le numéro complet
    }
    std::string mode() const override { return "Carte bancaire"; }
    std::string details() const override { return "**** " + quatreDerniers_ + ", " + titulaire_; }
private:
    std::string titulaire_, quatreDerniers_;
};

class PaiementMobileMoney : public Paiement {
public:
    PaiementMobileMoney(double montant, std::string operateur, const std::string& numero)
        : Paiement(montant), operateur_(std::move(operateur)), numero_(chiffresSeulement(numero)) {
        if (numero_.size() < 8 || numero_.size() > 13)
            throw ErreurMetier("Numéro Mobile Money invalide.");
    }
    std::string mode() const override { return "Mobile Money"; }
    std::string details() const override {
        return operateur_ + " ***" + numero_.substr(numero_.size() - 3);
    }
private:
    std::string operateur_, numero_;
};

class PaiementEspeces : public Paiement {
public:
    PaiementEspeces(double montant, std::string caissier)
        : Paiement(montant), caissier_(std::move(caissier)) {}
    std::string mode() const override { return "Espèces"; }
    std::string details() const override { return "guichet: " + caissier_; }
private:
    std::string caissier_;
};

// ---------------------------------------------------------------------------
// Pattern Strategy : la règle de remboursement est interchangeable.
// ---------------------------------------------------------------------------
class PolitiqueRemboursement {
public:
    virtual ~PolitiqueRemboursement() = default;
    // Taux de remboursement (0.0 à 1.0) selon le nombre d'heures avant le départ.
    virtual double taux(double heuresAvantDepart) const = 0;
};

class PolitiqueStandard : public PolitiqueRemboursement {
public:
    double taux(double h) const override {
        if (h >= 48) return 1.0;   // 100 % à 48 h ou plus
        if (h >= 24) return 0.5;   // 50 % entre 24 h et 48 h
        return 0.0;                // rien à moins de 24 h
    }
};
