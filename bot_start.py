#!/usr/bin/env python
# pylint: disable=unused-argument
# This program is dedicated to the public domain under the CC0 license.

"""
First, a few callback functions are defined. Then, those functions are passed to
the Application and registered at their respective places.
Then, the bot is started and runs until we press Ctrl-C on the command line.
Usage:
Example of a bot-user conversation using ConversationHandler.
Send /start to initiate the conversation.
Press Ctrl-C on the command line or send a signal to the process to stop the
bot.
"""

import logging

import pandas as pd
from telegram import ReplyKeyboardMarkup, ReplyKeyboardRemove, Update
from telegram.ext import (
    Application,
    CallbackContext,
    CommandHandler,
    ConversationHandler,
    MessageHandler,
    filters,
)

# Enable logging
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s", level=logging.INFO
)
logger = logging.getLogger(__name__)

NAME, PROBLEM, GOAL = range(3)




def recommend_products_by_user_features(skintone, skintype, eyecolor, haircolor, percentile=0.85):
    df = pd.read_csv('skindataall.csv', index_col=[0])
    ddf = df[(df['Skin_Tone'] == skintone) & (df['Hair_Color'] == haircolor) & (df['Skin_Type'] == skintype) & (df['Eye_Color'] == eyecolor)]

    recommendations = ddf[(ddf['Rating_Stars'].notnull())][['Rating_Stars', 'Product_Url', 'Product']]
    recommendations = recommendations.sort_values('Rating_Stars', ascending=False).head(10)

    print('Based on your features, these are the top products for you:')
    return recommendations, recommendations.index


async def start(update: Update, context: CallbackContext.DEFAULT_TYPE) -> int:
    """Starts the conversation and asks the user about their skinetype."""
    # reply_keyboard = [["Dark", "Deep", "Ebony", "Fair", "Light", "Medium", "Olive", "Porcelain", "Tan"]]

    await update.message.reply_text(
        "Привет! Я бот юридической клиники. "
        "Send /cancel to stop talking to me.\n\n"
        "Как тебя зовут?",

    )

    return NAME


async def name(update: Update, context: CallbackContext.DEFAULT_TYPE) -> int:
    """Stores the selected skintone and asks for a skintype."""
    global skintone_
    user = update.message.from_user
    logger.info("Name of %s: %s", user.first_name, update.message.text)

    skintone_ = update.message.text
    print(skintone_)
    # reply_keyboard = [["Combination", "Dry", "Normal", "Oily"]]
    await update.message.reply_text(
        "Приятно познакомиться :) Опиши свою проблему? ",

    )

    return PROBLEM


async def problem(update: Update, context: CallbackContext.DEFAULT_TYPE) -> int:
    """Stores the skintype and asks for an eyecolor."""
    global skintype_
    user = update.message.from_user
    logger.info("Skintype of %s: %s", user.first_name, update.message.text)

    skintype_ = update.message.text
    # reply_keyboard = [["Blue", "Brown", "Gray", "Green", "Hazel"]]
    await update.message.reply_text(
        "Чудесно! Что бы ты хотел сделать? ",

    )

    return GOAL


async def goal(update: Update, context: CallbackContext.DEFAULT_TYPE) -> int:
    """Stores the location and asks for some info about the user."""
    global eyecolor_
    user = update.message.from_user
    logger.info("Eyecolor of %s: %s", user.first_name, update.message.text)

    eyecolor_ = update.message.text
    print("Skintone:", skintone_, "Skintype:", skintype_, "Eyecolor:", eyecolor_)
    await update.message.reply_text("Спасибо за обращение в юридическую клинику!")



    df = pd.read_csv('results.csv')
    print(len(df.index))
    df.loc[len(df.index)] = [len(df.index), skintone_, skintype_, eyecolor_]

    df.to_csv('results.csv')

    print(df)

    # if (recommendations.empty):
    #     await update.message.reply_text("I have no idea")
    # else:
    #     await update.message.reply_text(recommendations['Product_Url'][index[0]])
    return ConversationHandler.END




async def cancel(update: Update, context: CallbackContext.DEFAULT_TYPE) -> int:
    """Cancels and ends the conversation."""
    user = update.message.from_user
    logger.info("User %s canceled the conversation.", user.first_name)
    await update.message.reply_text(
        "Пока!", reply_markup=ReplyKeyboardRemove()
    )

    return ConversationHandler.END


def main() -> None:
    """Run the bot."""
    # Create the Application and pass it your bot's token.
    application = Application.builder().token("5575555486:AAHs0_l9foFVl4awBYHb7zCnAnAcTTi4iZE").build()

    # Add conversation handler with the states SKINTONE, SKINTYPE, EYECOLOR, HAIRCOLOR
    conv_handler = ConversationHandler(
        entry_points=[CommandHandler("start", start)],
        states={
            NAME: [MessageHandler(filters.TEXT, name)],
            PROBLEM: [MessageHandler(filters.TEXT, problem)],
            GOAL: [MessageHandler(filters.TEXT, goal)],

        },
        fallbacks=[CommandHandler("cancel", cancel)],
    )


    application.add_handler(conv_handler)

    # Run the bot until the user presses Ctrl-C
    application.run_polling()


if __name__ == "__main__":
    main()