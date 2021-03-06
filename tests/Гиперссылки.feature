# language: ru
# encoding: utf-8
@UA30_Прочие_макеты

Функциональность: Браузер
	Как специалист по тестированию

Гиперссылки:
	* Контрагенты
		|"Имя"         |"Представление"       |"Значение"          |
		|"ИмяЭлемента" |"Имя ссылки элемента" |"Значение элемента" |
		|"ИмяРеквизита"|"Имя ссылки реквизита"|"Значение реквизита"|
	* Номенклатура
		|"Код"    |"Наименование"                  |"Единица" |
		|"Гвоздь" |"Гвоздь строительный 1,2x16 мм" |"шт."     |
		|"Труба"  |"Труба стальная 100 мм"         |"м"       |

Контекст:
	Дано Я запускаю сценарий открытия TestClient или подключаю уже существующий
	Открывается тест-клиент

Структура сценария:
	* Открытие формы элемента
		Если Версия платформы ">=" "8.3.6" Тогда
		И я показываю подсказку "ТекстПодсказки" EnjoyHint у элемента "ИмяЭлемента"
			|"shape"|"rect"|
			|"timeout"|"5000"
		И видеовставка картинки "Гвоздь" 'Труба'
		И в таблице "$ИмяТаблицы$" есть колонка 'ИмяРеквизита' Тогда
		"""
		И многострочный текст
		Если в панели открытых
		"""
		Если в панели открытых есть команда "ЗаписатьИЗакрыть" Тогда
		Если в таблице <Номенклатура> есть колонка с именем "ИмяКолонки" Тогда
		Если в окне предупреждения нет текста "Нужный текст" тогда
		Если в панели открытых есть команда "ЗаписатьИЗакрыть" Тогда
		И видеовставка картинки "ИмяКартинки" "ТекстДиктора"
	* Проверка шагов
		Если в сообщениях пользователю есть строка 'МояСтрока' Тогда
		К тому же в окне предупреждения нет текста 'Нужный текст' тогда
		Если в панели открытых есть команда 'ЗаписатьИЗакрыть' Тогда
		Если в таблице 'Номенклатура' есть колонка с именем 'ИмяКолонки' Тогда
		И видеовставка картинки "ИмяКартинки" "ТекстДиктора"
		И в таблице "$ИмяТаблицы$" есть колонка с именем '$ИмяРеквизита$' Тогда
		Если в панели открытых есть команда "ЗаписатьИЗакрыть" Тогда

