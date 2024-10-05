#pragma once
#include <QValidator>

class LimitingDoubleValidator : public QDoubleValidator
{
public:
	LimitingDoubleValidator(const double bottom, const double top, const int decimals, QObject* parent) : QDoubleValidator(bottom, top, decimals, parent)
	{
		setLocale(QLocale::C);
	}

	State validate(QString& s, int& i) const override
	{
		return s.isEmpty() || QDoubleValidator::validate(s, i) == Acceptable ? Acceptable : Invalid;
	}
};