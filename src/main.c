#include <zephyr/kernel.h>
#include "MKL25Z4.h"
#include <stdint.h>

/*
 * Leitura ADC na FRDM-KL25Z
 *
 * Entrada analógica:
 * PTB1 -> ADC0_SE9
 *
 * LEDs:
 * LED verde -> PTB19
 * LED azul  -> PTD1
 *
 * Como os LEDs são active low:
 * pino = 0 -> LED aceso
 * pino = 1 -> LED apagado
 */

#define ADC_CHANNEL_PTB1     9

#define GREEN_LED_PIN        19
#define BLUE_LED_PIN         1

#define GREEN_LED_MASK       (1U << GREEN_LED_PIN)
#define BLUE_LED_MASK        (1U << BLUE_LED_PIN)

#define LOW_THRESHOLD        20
#define HIGH_THRESHOLD       200

void delayMs(int n)
{
    volatile int i;
    volatile int j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < 21000; j++) {
            /* espera ativa */
        }
    }
}

static void init_leds(void)
{
    /*
     * Habilita clock das portas B e D.
     * Porta B: PTB19, LED verde.
     * Porta D: PTD1, LED azul.
     */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;

    /*
     * Configura PTB19 e PTD1 como GPIO.
     */
    PORTB->PCR[GREEN_LED_PIN] = PORT_PCR_MUX(1);
    PORTD->PCR[BLUE_LED_PIN]  = PORT_PCR_MUX(1);

    /*
     * Configura os pinos dos LEDs como saída.
     */
    PTB->PDDR |= GREEN_LED_MASK;
    PTD->PDDR |= BLUE_LED_MASK;

    /*
     * Inicialmente apaga os dois LEDs.
     * Como são active low, escrever 1 apaga.
     */
    PTB->PSOR = GREEN_LED_MASK;
    PTD->PSOR = BLUE_LED_MASK;
}

static void turn_green_on(void)
{
    PTB->PCOR = GREEN_LED_MASK;  // verde aceso
    PTD->PSOR = BLUE_LED_MASK;   // azul apagado
}

static void turn_blue_on(void)
{
    PTB->PSOR = GREEN_LED_MASK;  // verde apagado
    PTD->PCOR = BLUE_LED_MASK;   // azul aceso
}

static void turn_leds_off(void)
{
    PTB->PSOR = GREEN_LED_MASK;
    PTD->PSOR = BLUE_LED_MASK;
}

static void init_adc(void)
{
    /*
     * Habilita clock da porta B,
     * pois o canal analógico usado está no PTB1.
     */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    /*
     * Configura PTB1 como entrada analógica.
     * Para função analógica, o campo MUX deve ficar em 0.
     */
    PORTB->PCR[1] &= ~PORT_PCR_MUX_MASK;

    /*
     * Habilita clock do ADC0.
     */
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    /*
     * Usa gatilho por software.
     */
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;

    /*
     * Configura resolução e clock do ADC.
     *
     * MODE = 00 -> conversão em 8 bits
     * ADICLK = 11 -> clock assíncrono ADACK
     */
    ADC0->CFG1 &= ~ADC_CFG1_MODE_MASK;
    ADC0->CFG1 |= ADC_CFG1_ADICLK(3);
}

static uint16_t read_adc(uint8_t channel)
{
    /*
     * Escrever em SC1 inicia a conversão no canal escolhido.
     */
    ADC0->SC1[0] = ADC_SC1_ADCH(channel);

    /*
     * Aguarda a conversão terminar.
     * COCO = Conversion Complete Flag.
     */
    while ((ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0) {
        /* espera fim da conversão */
    }

    /*
     * Lê o resultado da conversão.
     */
    return ADC0->R[0];
}

static void update_leds_from_adc(uint16_t adc_value)
{
    if (adc_value <= LOW_THRESHOLD) {
        turn_green_on();
    } else if (adc_value >= HIGH_THRESHOLD) {
        turn_blue_on();
    } else {
        turn_leds_off();
    }
}

void main(void)
{
    uint16_t adc_value;

    init_leds();
    init_adc();

    printk("Iniciando leituras do ADC...\n");

    while (1) {
        adc_value = read_adc(ADC_CHANNEL_PTB1);

        printk("SAIDA: %u\n", adc_value);

        update_leds_from_adc(adc_value);

        delayMs(200);
    }
}