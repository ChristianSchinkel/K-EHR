#include "Patient.h"

namespace ehr {

Patient::Patient(std::string firstName, std::string lastName,
                 std::string dob, std::string gender,
                 std::string mrn)
    : firstName_(std::move(firstName))
    , lastName_(std::move(lastName))
    , dob_(std::move(dob))
    , gender_(std::move(gender))
    , mrn_(std::move(mrn))
{}

std::string Patient::fullName() const {
    return firstName_ + " " + lastName_;
}

} // namespace ehr
