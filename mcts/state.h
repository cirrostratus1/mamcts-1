// Copyright (c) 2019 Julian Bernhard
// 
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.
// ========================================================

#ifndef MCTS_STATE_H
#define MCTS_STATE_H

#include "boost/polymorphic_cast.hpp"
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>
#include "common.h"


namespace mcts {


typedef unsigned char ActionIdx;
typedef unsigned char AgentIdx;
typedef std::vector<ActionIdx> JointAction;

typedef double Reward;

    template <typename T>
    std::vector<T> operator+(const std::vector<T>& a, const std::vector<T>& b)
    {
        TEST_ASSERT(a.size() == b.size());

        std::vector<T> result;
        result.reserve(a.size());

        std::transform(a.begin(), a.end(), b.begin(),
                       std::back_inserter(result), std::plus<T>());
        return result;
    }

    template <typename T>
    std::vector<T>& operator+=(std::vector<T>& a, const std::vector<T>& b)
    {
        TEST_ASSERT(a.size() == b.size());

        for(int i=0; i<a.size(); i++){
            a[i] = a[i] + b[i];
        }
        
        
        //std::transform(a.begin(), a.end(), b.begin(),
        //               a.begin(), std::plus<T>());
        return a;
    }

    std::ostream& operator<<(std::ostream& os, const JointAction& a)
    {
        os << "[";
        for (auto it = a.begin(); it != a.end(); ++it)
            os << (int)(*it) << " ";
        os << "]";
        return os;
    }


    template<typename Implementation>
class StateInterface {
public:

    std::shared_ptr<Implementation> execute(const JointAction &joint_action, std::vector<Reward>& rewards) const;

    std::shared_ptr<Implementation> clone() const;

    ActionIdx get_num_actions(AgentIdx agent_idx) const;

    bool is_terminal() const;

    const std::vector<AgentIdx> get_agent_idx() const;

    static const AgentIdx ego_agent_idx;

    std::string sprintf() const;

    virtual ~StateInterface() {};

private:
    CRTP_INTERFACE(Implementation)
    CRTP_CONST_INTERFACE(Implementation)


};

template<typename Implementation>
inline std::shared_ptr<Implementation> StateInterface<Implementation>::execute(const JointAction &joint_action, std::vector<Reward>& rewards) const {
   return impl().execute(joint_action, rewards);
}

template<typename Implementation>
inline std::shared_ptr<Implementation> StateInterface<Implementation>::clone() const {
 return impl().clone();
}

template<typename Implementation>
inline ActionIdx StateInterface<Implementation>::get_num_actions(AgentIdx agent_idx) const {
    return impl().get_num_actions(agent_idx);
}

template<typename Implementation>
inline bool StateInterface<Implementation>::is_terminal() const {
    return impl().is_terminal();
}

template<typename Implementation>
inline const std::vector<AgentIdx> StateInterface<Implementation>::get_agent_idx() const {
    return impl().get_agent_idx();
}


template<typename Implementation>
inline std::string StateInterface<Implementation>::sprintf() const {
    return impl().sprintf();
}


template<typename Implementation>
const AgentIdx StateInterface<Implementation>::ego_agent_idx = 0;



} // namespace mcts

#endif