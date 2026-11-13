#pragma once
#include <sstream>
#include <string>
#include <utility>
#include "Utils.hpp"

class Passager {
public:
    Passager(int id, std::string nom, std::string prenom,
             std::string passeport, std::string telephone)
        : id_(id), nom_(std::move(nom)), prenom_(std::move(prenom)),
          passeport_(std::move(passeport)), telephone_(std::move(telephone)) {
        if (nom_.empty() || prenom_.empty()) throw ErreurMetier("Nom et prénom obligatoires.");
        if (passeport_.empty()) throw ErreurMetier("Numéro de passeport obligatoire.");
    }

    int id() const { return id_; }
    const std::string& nom() const { return nom_; }
    const std::string& prenom() const { return prenom_; }
    const std::string& passeport() const { return passeport_; }
    const std::string& telephone() const { return telephone_; }

    void changerTelephone(std::string tel) { telephone_ = std::move(tel); }

    std::string resume() const {
        std::ostringstream os;
        os << "#" << id_ << " " << prenom_ << " " << nom_
           << " | passeport: " << passeport_ << " | tél: " << telephone_;
        return os.str();
    }

private:
    int id_;
    std::string nom_, prenom_, passeport_, telephone_;
};
