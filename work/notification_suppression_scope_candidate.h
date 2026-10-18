#pragma once
#include <cstdint>
struct NotificationSuppressionScopeCandidate {std::uint8_t saved=0,restored=0;};
template<class Service>
inline void beginNotificationSuppressionScopeCandidate(NotificationSuppressionScopeCandidate& scope,Service& service) {
    scope.restored=0;
    scope.saved=service.getSuppression();
    service.setSuppression(1);
}
template<class Service>
inline void restoreNotificationSuppressionScopeCandidate(NotificationSuppressionScopeCandidate& scope,Service& service) {
    if(scope.restored)return;
    service.setSuppression(scope.saved);
    scope.restored=1;
}
