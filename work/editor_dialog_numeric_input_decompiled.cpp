// Recovered dialog numeric getter behavior (Qt4 API names verified).
// float: bounds -> type5 text().toFloat(&ok) writes only if ok;
// type6: float(QDoubleSpinBox::value()) succeeds; otherwise false.
// int: bounds -> type7 QSpinBox::value() writes int32; otherwise false.
// No std::stof substitution: Qt parsing/validators must be preserved.
