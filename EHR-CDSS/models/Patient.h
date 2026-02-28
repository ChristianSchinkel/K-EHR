#pragma once

#include <string>
#include <ctime>

namespace ehr {

/**
 * @brief Represents a patient in the EHR system.
 */
class Patient {
public:
    Patient() = default;
    Patient(std::string firstName, std::string lastName,
            std::string dob, std::string gender,
            std::string mrn);

    /* Accessors */
    int               getId()        const { return id_; }
    const std::string& getFirstName() const { return firstName_; }
    const std::string& getLastName()  const { return lastName_; }
    const std::string& getDob()       const { return dob_; }
    const std::string& getGender()    const { return gender_; }
    const std::string& getMrn()       const { return mrn_; }

    /* Mutators */
    void setId(int id)                          { id_ = id; }
    void setFirstName(const std::string& v)     { firstName_ = v; }
    void setLastName(const std::string& v)      { lastName_  = v; }
    void setDob(const std::string& v)           { dob_       = v; }
    void setGender(const std::string& v)        { gender_    = v; }
    void setMrn(const std::string& v)           { mrn_       = v; }

    std::string fullName() const;

private:
    int         id_        = 0;
    std::string firstName_;
    std::string lastName_;
    std::string dob_;
    std::string gender_;
    std::string mrn_;
};

} // namespace ehr
