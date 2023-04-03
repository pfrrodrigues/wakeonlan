#pragma once
#include <mutex>
#include <string>
#include <memory>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace WakeOnLanImpl {
    /**
     * @class Table
     * This class is the generic representation of a group of participants. Every ::Participant being part of a
     * table represents a host on the local network that is subscribed and connected to the service managed by an
     * instance of the API running a manager handler. Operations over the table are realized by the services supported
     * by the ::ManagerHandler struct.
     */
    class Table {
    public:
        /**
         * @enum ParticipantStatus
         * The participant status represents the manager point of view in relation to a
         * participant that is part of the group managed by it. The type composes
         * the Participant struct and is the field manipulated by the ::MonitoringService when
         * calling the ::Table::update() function.
         */
        enum class ParticipantStatus {
            Awaken = 0,       ///< The participant is answering to the services requests.
            Sleeping = 1,     ///< The participant is part of the group and is not answering to the service requests.
            Unknown = 2,       ///< The initial state of a added participant. The state changes to Awaken after manager receives a response to SleepStatusRequest.
            Manager = 3       ///< The initial state of a added participant. The state changes to Awaken after manager receives a response to SleepStatusRequest.
        };

        /**
         * @struct Participant
         * A struct used to hold the information of a participant on the ::Table. Participants
         * are created and inserted on the group table by the API services. Each participant
         * contains the hostname, IP address, MAC address, and status information.
         */
        struct Participant {
            std::string electedTimestamp;
            std::string hostname;       ///< The participant hostname.
            std::string ip;             ///< The participant IP address.
            std::string mac;            ///< The participant MAC address.
            ParticipantStatus status;   ///< The participant status.
        };

        /**
       * Gets a reference of ::Table. ::Table is a singleton class, so then
       * every call to the method will returns a reference to the same object.
       *
       * @returns A ::Table reference of a singleton instance.
       */
        static Table &get();

        /**
        * Inserts a previously created Participant. Is assumed the fields of the Participant
        * instance being passed as argument was well set to the right values before call it.
        * The function does not check any field before try to insert in on the data structure
        * representing the table. The true value is returned to the caller when the participant
        * was successfully inserted on the table. Otherwise, the false value is returned.
        *
        * @param participant A Participant reference representing the participant that must be inserted on the table.
        * @returns A bool indicating the insertion on the participant on the table was successful.
        */
        std::pair<uint32_t, std::vector<Participant>> insert(const Participant &participant);

        bool transaction(const uint32_t & seqNo, const std::vector<Participant> & tbl);

        /**
        * Updates the status of a participant of the table. The function checks if the
        * participant is in fact being part of the group. In case of that, the new
        * status is assigned to the participant status and the true value is returned to the
        * caller of the function. Otherwise, false is returned, indicating the update
        * did not occur with success.
        *
        * @param status A reference to an ParticipantStatus type to assigned to the participant
        * @param hostname A string reference.
        * @returns A bool indicating the update on the participant status was successful
        */
        std::pair<uint32_t, std::vector<Table::Participant>> update(const ParticipantStatus &status, const std::string &hostname);

        /**
        * Removes of the table a previously inserted Participant. The function checks if the
        * participant is in fact being part of the group. In case of that, the participant is removed
        * of the group and the true value is returned to the caller of the function. Otherwise,
        * false is returned, indicating the deletion did not occur with success.
        *
        * @param hostname The hostname of the participant to be removed.
        * @return A bool indicating the participant was deleted of the table with success.
        */
        std::pair<uint32_t, std::vector<Table::Participant>> remove(const std::string &hostname);

        /**
         * Gets all participants registered in the table.
         * 
         * @return A vector of registered participants.  
         */
        std::vector<Participant> get_participants_monitoring();

        /**
         * Gets all participants registered in the table.
         * The function is operates on a blocking mode. That means
         * once the function is called, it only returns the control
         * to the caller after the table is updated by an event.
         * @return A vector of registered participants.
         */
        std::vector<Participant> get_participants_interface();

        /**
         * Gets the current manager as in the last update of the present table.
         * @return The Participant object that is the current Manager if succesfull,
         * an empty Participant if otherwise.
         */
        Participant get_manager();
    private:
        Table() = default;

        ~Table() = default;

        Table(const Table &table);

        const Table &operator =(const Table &table);

        std::mutex tableMutex;                              ///< The mutex to manage access to the table representation.
        std::shared_ptr<spdlog::logger> log;                ///< The Table logger.
        std::unordered_map<std::string, Participant> data;  ///< The table representation.
        uint32_t seq;
    };
} // namespace WakeOnLanImpl